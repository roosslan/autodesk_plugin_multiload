#pragma once

#include "stdafx.h"
#include "context_handler.h"
#include "sensitive_data.h"
#include "isolated_load_context.h"

namespace bi_loader {

    public value struct addin_update {
        int button_id;
        int parent_button;
        string button_type;
        string button_text;
        string command;
        string large_image;
        string image;
        string tool_tip;
        string panel_name;
        string version;
    };

    [Transaction(TransactionMode::Manual)]
    public ref class host_app : IExternalApplication {
        string temp_dll_path_;        
        Type^ app_type_;
        IExternalApplication^ revit_addin_iapp_;
        wrapper_into_context^ api_wrapper_;
        isolated_load_context^ loaded_context_;
        ExternalEvent^ external_export_event_;        
        UIApplication^ ext_ui_application_;
        void delegate_on_application_initialized(object sender, ApplicationInitializedEventArgs^ e);        

/*      Assembly^ Load(AssemblyLoadContext^ assembly_load_context, string file_path);
        void delegate_component_manager_ui_element_activated(object sender, UIElementActivatedEventArgs^ e);  */
    public:
        host_app();

        static UIControlledApplication^ uic_application;
        static string assembly_location;
        static Assembly^ loaded_assembly;
        
        static array<Type^>^ commands;
        static array<addin_update>^ get_version_from_db();
        static string last_loaded_ver;
        
        static List<addin_update>^ buttons_list = gcnew List<addin_update>();

        static const string biloader_dll_path = Assembly::GetExecutingAssembly()->Location;
        static const string biloader_dll_directory = Path::GetDirectoryName(biloader_dll_path);

        static const string main_addin_dll_name = "revit_addin26.dll";
        static const string addin_class_name = "revit_addin26";
        static const string temp_path = Environment::ExpandEnvironmentVariables("%LocalAppData%\\Temp" );
        
        static const string app_data_directory = Environment::GetFolderPath(Environment::SpecialFolder::ApplicationData);
        
        static string aes_decode(string obf);
        static void create_ribbon_buttons();
        static int collect_buttons();
        static void clear_panels(string tab_name);
        static void copy_directory(string source_dir, string dest_dir);        
        static UI::RibbonPanel^ get_panel(string tab_name, string panel_name);
        static bool is_netpath_available(const std::wstring& path, int timeout_ms);

        Result DllMain(UIControlledApplication^ hinst_dll);
        virtual Result OnStartup(UIControlledApplication^ ui_app);
        virtual Result OnShutdown(UIControlledApplication^ ui_app);
    };
}
