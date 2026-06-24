#include "launcher_button.h"
#include "host_app.h"
#include "sensitive_data.h"

namespace bi_loader {

    static Assembly^ load(AssemblyLoadContext^ assembly_load_context, string file_path) {
        const auto stream = gcnew FileStream(file_path, FileMode::Open, FileAccess::Read);
        return assembly_load_context->LoadFromStream(stream);
    }

    Result btn_click::Execute(ExternalCommandData^ command_data, string% message, ElementSet^ elements) {
        /* Сюда ссылается ф-ция host_app::create_ribbon_buttons() ...  gcnew PushButtonData("ID_UPDATE26_BUTTON" ...); */

		const auto upd_directory = gcnew String(update_directory);
        try {
            if (host_app::is_netpath_available(update_directory, 5000)) {
                string assembly_directory = upd_directory;
                string actual_dll_path = Path::Combine(assembly_directory, host_app::main_addin_dll_name);

                /* Create a unique temp directory for this session */
                string temp_directory = host_app::temp_path + "\\revit_addin\\comm\\" + Guid::NewGuid().ToString();
                Directory::CreateDirectory(temp_directory);

                /* Copy all files to temp directory */
                bi_loader::host_app::copy_directory(assembly_directory, temp_directory);

                /* Load the assembly from temp directory */
                const auto comm_dll_path = Path::Combine(temp_directory, host_app::main_addin_dll_name);               
                const auto assebmly_version_info = System::Diagnostics::FileVersionInfo::GetVersionInfo(comm_dll_path);

                host_app::last_loaded_ver = assebmly_version_info->FileVersion;
                const auto loaded_version = gcnew Version(host_app::last_loaded_ver);

                host_app::collect_buttons();
                /* remove_button(bim_panel, "BIM Profile"); */

                auto new_commands = gcnew List<addin_update>();
                /* Выбираем из массива команд БД только подходящие (новые, новее) */
                auto upd_rows = host_app::get_version_from_db();
                if (upd_rows->Length != 0)
                    for each (const auto db_command in upd_rows) {
                        try {
                        	auto row_version = gcnew Version(db_command.version);
                            if (row_version > loaded_version) {
                                new_commands->Add(db_command);
                            }
                        }
                        catch (exception ex) {
                            UI::TaskDialog::Show("Failed to compare versions! ", ex->Message + "\n\n" + ex->StackTrace);
                        }
                    }

                const auto bim_panel = gcnew String(wpanel_name);
                host_app::clear_panels(bim_panel);

                /* Создаём контролы, полученные из БД */
                for (int i = 0; i < new_commands->Count; ++i) {
                    /* Обычная кнопка */
                    if (new_commands[i].button_type == "pushbutton" && new_commands[i].parent_button == 0) {
                        auto btn_runcmd_dll = gcnew PushButtonData(Guid::NewGuid().ToString(), new_commands[i].button_text, comm_dll_path, new_commands[i].command);
                        btn_runcmd_dll->LargeImage = gcnew BitmapImage(gcnew Uri(temp_directory + "\\" + new_commands[i].large_image));
                        btn_runcmd_dll->Image = gcnew BitmapImage(gcnew Uri(temp_directory + "\\" + new_commands[i].image));
                        btn_runcmd_dll->AvailabilityClassName = "revit_addin26.Utilities.Availability";
                        dynamic_cast<PushButton^>(host_app::get_panel(bim_panel, new_commands[i].panel_name)->AddItem(btn_runcmd_dll));
                    }
                    /* Pulldown-button */
                    if (new_commands[i].button_type == "pulldown" && new_commands[i].parent_button == 0) {
                        auto btn_pulldown_data = gcnew PulldownButtonData(Guid::NewGuid().ToString(), new_commands[i].button_text);
                        btn_pulldown_data->LargeImage = gcnew BitmapImage(gcnew Uri(temp_directory + "\\" + new_commands[i].large_image));
                        btn_pulldown_data->Image = gcnew BitmapImage(gcnew Uri(temp_directory + "\\" + new_commands[i].image));
                        auto btn_pulldown = dynamic_cast<PulldownButton^>(host_app::get_panel(bim_panel, new_commands[i].panel_name)->AddItem(btn_pulldown_data));

						for each (const auto child_button in new_commands)	{
							if (child_button.parent_button == new_commands[i].button_id) {
                                auto btn_child = gcnew PushButtonData(Guid::NewGuid().ToString(), child_button.button_text, comm_dll_path, child_button.command);
                                btn_child->LargeImage = gcnew BitmapImage(gcnew Uri(temp_directory + "\\" + child_button.large_image));
                                btn_child->Image = gcnew BitmapImage(gcnew Uri(temp_directory + "\\" + child_button.image));
                                btn_child->AvailabilityClassName = "revit_addin26.Utilities.Availability";
                                btn_pulldown->AddPushButton(btn_child);
							}
						}
                    }
                }
            }
        }
        catch (exception ex) {
            UI::TaskDialog::Show("Failed to install new commands! ", ex->Message + "\n\n" + ex->StackTrace);
        }
        return Result::Succeeded;
	}

	bool btn_availability::IsCommandAvailable(UIApplication^ uiapp, CategorySet^ cat_set) {
			return true;
    }
}
