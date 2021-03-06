/**************************************************************************
 Copyright:
      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#include <string>
#include <locale>  // std::locale
#include <cstdio>  // std::printf
#include <vector>
#include <sstream>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtk/gtk.h>  // gtk_window_set_default_icon_name, gtk_icon_theme_*
#include <glib.h>  // g_, G*
#include <glibmm/refptr.h>
#include <glibmm/miscutils.h>  // set_application_name
#include <glibmm/thread.h>  // thread_init
// #include <gtkmm.h>
// #include <iostream>

#include "libdebug/libdebug.h"  // include full libdebug here (to add domains, etc...)
#include "rconfig/rconfig.h"  // include full rconfig here
#include "hz/data_file.h"  // data_file_add_search_directory
#include "hz/fs_tools.h"  // get_home_dir
#include "hz/fs_path.h"  // FsPath
#include "hz/hz_config.h"  // ENABLE_GLIB
#include "hz/string_algo.h"  // string_join

#include "gsc_main_window.h"
#include "gsc_executor_log_window.h"
#include "gsc_settings.h"
#include "gsc_init.h"



namespace {

	static std::string s_home_config_file;


	static debug_channel_base_ptr s_debug_buf_channel;
	static std::stringstream s_debug_buf_channel_stream;


	// This function is not thread-safe. The channel must be locked properly.
	inline debug_channel_base_ptr app_get_debug_buf_channel()
	{
		if (!s_debug_buf_channel)
			return (s_debug_buf_channel = new DebugChannelOStream(s_debug_buf_channel_stream));
		return s_debug_buf_channel;
	}

}


// This should be called from one thread only.
std::string app_get_debug_buffer_str()
{
	debug_channel_base_ptr channel = app_get_debug_buf_channel();
	DebugChannelOStream::intrusive_ptr_scoped_lock_type locker(channel->get_ref_mutex());  // lock
	// now the channel is locked, so we have a proper access to the stream.
	return s_debug_buf_channel_stream.str();
}






inline bool app_init_config()
{
#ifdef _WIN32
	std::string global_config_file = "gsmartcontrol.conf";  // installation dir
	s_home_config_file = hz::get_home_dir() + hz::DIR_SEPARATOR_S
			+ "gsmartcontrol.conf";  // $HOME/program_name.conf
#else
	std::string global_config_file = std::string(PACKAGE_SYSCONF_DIR)
			+ hz::DIR_SEPARATOR_S + "gsmartcontrol.conf";
	s_home_config_file = hz::get_home_dir() + hz::DIR_SEPARATOR_S + ".gsmartcontrolrc";
#endif

	hz::FsPath gp(global_config_file);  // Default system-wide settings. This file is empty by default.
	hz::FsPath hp(s_home_config_file);  // Per-user settings.

	if (gp.exists() && gp.is_readable())
		rconfig::load_from_file(gp.str());

	if (hp.exists() && hp.is_readable())
		rconfig::load_from_file(hp.str());

	rconfig::dump_tree();

	init_default_settings();  // initialize /default

#ifdef ENABLE_GLIB
	rconfig::autosave_set_config_file(s_home_config_file);
	uint32_t autosave_timeout = rconfig::get_data<uint32_t>("system/config_autosave_timeout");
	if (autosave_timeout)
		rconfig::autosave_start(autosave_timeout);
#endif

	return true;
}



inline void app_show_first_boot_message(Gtk::Window* parent)
{
	bool first_boot = false;
	rconfig::get_data("system/first_boot", first_boot);

	if (first_boot) {
// 		Glib::ustring msg = "First boot";
// 		Gtk::MessageDialog(*parent, msg, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true).run();
	}

// 	rconfig::set_data("system/first_boot", false);  // don't show it again
}




// glib message -> libdebug message convertor
extern "C" {
	static void glib_message_handler(const gchar* log_domain, GLogLevelFlags log_level,
			const gchar* message, gpointer user_data)
	{
		// log_domain is already printed as part of message.
		debug_print_error("gtk", "%s\n", message);
	}
}


// Note: Use GLib types here.
struct CmdArgs {
	CmdArgs() :
		// defaults
		arg_locale(TRUE),
		arg_version(FALSE),
		arg_scan(TRUE),
		arg_hide_tabs(TRUE),
		arg_add_virtual(NULL),
		arg_add_device(NULL)
	{ }

	gboolean arg_locale;  // if false, disable locale
	gboolean arg_version;  // if true, show version and exit
	gboolean arg_scan;  // if false, don't scan the system for drives on startup
	gboolean arg_hide_tabs;  // if true, hide additional info tabs when smart is disabled. false may help debugging.
	gchar** arg_add_virtual;  // load smartctl data from these files as virtual drives
	gchar** arg_add_device;  // add these device files manually
};



inline bool parse_cmdline_args(CmdArgs& args, int& argc, char**& argv)
{
	static const GOptionEntry arg_entries[] =
	{
		{ "no-locale", 'l', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_locale), "Disable locale", NULL },
		{ "version", 'V', 0, G_OPTION_ARG_NONE, &(args.arg_version), "Display version information", NULL },
		{ "no-scan", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_scan), "Don't scan devices on startup", NULL },
		{ "no-hide-tabs", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &(args.arg_hide_tabs), "Don't hide non-identity tabs when SMART is disabled. Useful for debugging.", NULL },
		{ "add-virtual", '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &(args.arg_add_virtual), "Load smartctl data from file, creating a virtual drive", NULL },
		{ "add-device", '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &(args.arg_add_device), "Add this device to device list. Useful with --no-scan to list certain drives only.", NULL },
		{ NULL }
	};

	GError* error = 0;
	GOptionContext* context = g_option_context_new("- A GTK+ GUI for smartmontools");
	g_option_context_add_main_entries(context, arg_entries, NULL);
	g_option_context_add_group(context, gtk_get_option_group(false));  // gtk options

	// libdebug options; this will also automatically apply them
	g_option_context_add_group(context, debug_get_option_group());
	g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);

	return true;
}



inline void app_print_version_info()
{
	std::string versiontext = std::string("\nGSmartControl version ") + VERSION + "\n";

	std::string warningtext = std::string("\nWarning: GSmartControl");
	warningtext += " comes with ABSOLUTELY NO WARRANTY.\n";
	warningtext += "See LICENSE_gsmartcontrol.txt file for details.\n";
	warningtext += "\nCopyright (C) 2008  Alexander Shaduri <ashaduri" "" "@" "" "" "gmail.com>\n\n";

	std::printf("%s%s", versiontext.c_str(), warningtext.c_str());
}



bool app_init_and_loop(int& argc, char**& argv)
{

	// glib needs this for command line args. It will be reset by Gtk::Main later.
	std::locale::global(std::locale(""));  // set locale to system LANG


	// initialize GThread (for mutexes, etc... to work). Must be called before any other glib function.
	Glib::thread_init();


	// parse command line args
	CmdArgs args;
	if (! parse_cmdline_args(args, argc, argv)) {
		return true;
	}

	if (args.arg_version) {
		// show version information and exit
		app_print_version_info();
		return true;
	}


	// register libdebug domains
	debug_register_domain("gtk");
	debug_register_domain("app");
	debug_register_domain("hz");
	debug_register_domain("rmn");
	debug_register_domain("rconfig");


	// Add special debug channel to collect all libdebug output into a buffer.
	debug_add_channel("all", debug_level::all, app_get_debug_buf_channel());



	std::vector<std::string> load_virtuals;
	if (args.arg_add_virtual) {
		const gchar* entry = 0;
		while ( (entry = *(args.arg_add_virtual)++) != NULL ) {
			load_virtuals.push_back(entry);
		}
	}
	std::string load_virtuals_str = hz::string_join(load_virtuals, ", ");  // for display purposes only

	std::vector<std::string> load_devices;
	if (args.arg_add_device) {
		const gchar* entry = 0;
		while ( (entry = *(args.arg_add_device)++) != NULL ) {
			load_devices.push_back(entry);
		}
	}
	std::string load_devices_str = hz::string_join(load_devices, ", ");  // for display purposes only


	// it's here because earlier there are no domains
	debug_out_dump("app", "Application options:\n"
		<< "\tlocale: " << args.arg_locale << "\n"
		<< "\tversion: " << args.arg_version << "\n"
		<< "\thide_tabs: " << args.arg_hide_tabs << "\n"
		<< "\tscan: " << args.arg_scan << "\n"
		<< "\targ_add_virtual: " << (load_virtuals_str.empty() ? "[empty]" : load_virtuals_str) << "\n"
		<< "\targ_add_device: " << (load_devices_str.empty() ? "[empty]" : load_devices_str) << "\n");

	debug_out_dump("app", "LibDebug options:\n" << debug_get_cmd_args_dump());


	// Load config files
	app_init_config();


	// Initialize GTK+
	Gtk::Main m(argc, argv, args.arg_locale);


	// Redirect all GTK+/Glib and related messages to libdebug
	static const char* const gtkdomains[] = {
			// no atk or cairo, they don't log. libgnomevfs may be loaded by gtk file chooser.
			"GLib", "GModule", "GLib-GObject", "GLib-GRegex", "GLib-GIO", "GThread",
			"Pango", "Gtk", "Gdk", "GdkPixbuf", "libglade", "libgnomevfs",
			"glibmm", "giomm", "atkmm", "pangomm", "gdkmm", "gtkmm", "libglademm" };

	for (unsigned int i = 0; i < G_N_ELEMENTS(gtkdomains); ++i) {
		g_log_set_handler(gtkdomains[i], GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
				| G_LOG_FLAG_RECURSION), glib_message_handler, NULL);
	}


	// This shows up in About dialog gtk.
	Glib::set_application_name("GSmartControl");  // should be localized


	// Add data file search paths
#ifdef _WIN32
	hz::data_file_add_search_directory(".");  // the program is bundled with all the files in the same directory
#else
	#ifdef DEBUG_BUILD
		hz::data_file_add_search_directory(std::string(TOP_SRC_DIR) + "/src/res");  // application data resources
		hz::data_file_add_search_directory(std::string(TOP_SRC_DIR) + "/data");  // application data resources
	#else
		hz::data_file_add_search_directory(PACKAGE_PKGDATA_DIR);  // /usr/share/program_name
	#endif
#endif


	// set default icon for all windows.
	// set_default_icon_name is available since 2.12 in gtkmm, but since 2.6 in gtk.

	// we load it via icontheme to provide multi-size version.
	GtkIconTheme* default_icon_theme = gtk_icon_theme_get_default();

	// application-installed, /usr/share/icons/<theme_name>/apps/<size>
	if (gtk_icon_theme_has_icon(default_icon_theme, "gsmartcontrol")) {
		gtk_window_set_default_icon_name("gsmartcontrol");

	// try the gnome icon, it's higher quality / resolution
	} else if (gtk_icon_theme_has_icon(default_icon_theme, "gnome-dev-harddisk")) {
		gtk_window_set_default_icon_name("gnome-dev-harddisk");

	// gtk built-in, always available
	} else {
		gtk_window_set_default_icon_name("gtk-harddisk");
	}


	// Export some command line arguments to rmn

	// obey the command line option for no-scan on startup
	rconfig::set_data("/runtime/gui/force_no_scan_on_startup", !bool(args.arg_scan));

	// load virtual drives on startup if specified.
	rconfig::set_data("/runtime/gui/add_virtuals_on_startup", load_virtuals);

	// add devices to the list on startup if specified.
	rconfig::set_data("/runtime/gui/add_devices_on_startup", load_devices);

	// hide tabs if SMART is disabled
	rconfig::set_data("/runtime/gui/hide_tabs_on_smart_disabled", bool(args.arg_hide_tabs));


	// Create executor log window, but don't show it.
	// It will track all command executor outputs.
	GscExecutorLogWindow::create();


	// Open the main window
	GscMainWindow* win = GscMainWindow::create();
	if (!win) {
		debug_out_fatal("app", "Cannot create the main window. Exiting.\n");
		return false;  // cannot create main window
	}


	// first-boot message
	app_show_first_boot_message(win);


	// The Main Loop (tm)
	debug_out_info("app", "Entering main loop.\n");
	m.run();
	debug_out_info("app", "Main loop exited.\n");


	// close the main window and delete its object
	GscMainWindow::destroy();

	GscExecutorLogWindow::destroy();


	// std::cerr << app_get_debug_buffer_str();  // this will output everything that went through libdebug.


	return true;
}




void app_quit()
{
	debug_out_info("app", "Saving config before exit...\n");

	// save the config
#ifdef ENABLE_GLIB
	rconfig::autosave_force_now();
#else
	rconfig::save_to_file(s_home_config_file);
#endif

	// exit the main loop
	debug_out_info("app", "Trying to exit the main loop...\n");

	Gtk::Main::quit();

	// don't destroy main window here - we may be in one of its callbacks
}







