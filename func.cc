#include "stdafx.h"
#include "host_app.h"

using namespace bi_loader;

/* если nullptr, то возвращаем "" */
template<typename T>
string notnull(T value) {
    return value != nullptr ? value->ToString() : String::Empty;
}

array<addin_update>^ host_app::get_version_from_db()
try {
    auto updates_list = gcnew List<addin_update>();

	const auto db_hostname = gcnew String(wdb_hostname);
    const auto db_name = gcnew String(wdb_name);
    const auto db_user = gcnew String(wdb_user);
    const auto db_passwd = gcnew String(wdb_passwd);

    const string connection_string = "data source=" + db_hostname + ";initial catalog=" + db_name + ";user id=" + db_user + ";password=" + db_passwd + ";";

    auto conn = gcnew SqlConnection(connection_string);
    conn->Open();
    const string sql_query = "select button_id, parent_button, button_type, button_text, command, large_image, image, tool_tip, panel_name, version from addin_updates";
    auto sqlcmd = gcnew SqlCommand(sql_query, conn);
    auto reader = sqlcmd->ExecuteReader();

    while (reader->Read()) {
        addin_update update;
        update.button_id = Convert::ToInt32(reader["button_id"]);
        update.parent_button = Convert::ToInt32(reader["parent_button"]);
        update.button_type = reader["button_type"]->ToString();
        update.button_text = reader["button_text"]->ToString();
        update.command = reader["command"]->ToString();
        update.large_image = reader["large_image"]->ToString();
        update.image = reader["image"]->ToString();
        update.tool_tip = reader["tool_tip"]->ToString();
        update.panel_name = reader["panel_name"]->ToString();
        update.version = reader["version"]->ToString();

        updates_list->Add(update);
    }

    reader->Close();
    conn->Close();

    return updates_list->ToArray();
}
catch (exception ex) {
    (void)ex;
    /* TODO: read from txt file */
    try {

    }
    catch (...) {
        return gcnew array<addin_update>(0);
    }
    return gcnew array<addin_update>(0);
}


void host_app::copy_directory(const string source_dir, const string dest_dir) {
    /* Create destination directory if it doesn't exist */
    Directory::CreateDirectory(dest_dir);

    /* Copy all files */
    for each (const string file in Directory::GetFiles(source_dir)) {
        const string file_name = Path::GetFileName(file);
        const string dest_file = Path::Combine(dest_dir, file_name);

        try {
            File::Copy(file, dest_file, true);
        }
        catch (...) {
            /* Ignore copy errors for locked files */
        }
    }

    /* Copy all subdirectories */
    for each (const string dir in Directory::GetDirectories(source_dir)) {
        const string dir_name = Path::GetFileName(dir);
        const string dest_sub_dir = Path::Combine(dest_dir, dir_name);
        copy_directory(dir, dest_sub_dir);
    }
}

/* Собираем в массив все кнопки с их данными со вкладки BIM */
int host_app::collect_buttons() {
    RibbonControl^ ribbon_control = ComponentManager::Ribbon;
    auto commands = host_app::commands;

    buttons_list->Clear();

    const auto tab_name = gcnew String(wtab_name);
    for each (RibbonTab^ tab in ribbon_control->Tabs) {
        if (tab->Name == tab_name) {
            for each (Autodesk::Windows::RibbonPanel^ rib_panel in tab->Panels) {
                auto ui_application_type = UIApplication::typeid;
                try {
                    auto ribbon_items_property = ui_application_type->GetProperty("RibbonItemDictionary",
                        BindingFlags::NonPublic | BindingFlags::Static | BindingFlags::DeclaredOnly);

                    auto ribbon_items = static_cast<Dictionary<string, dict_ribbon_panel>^>(ribbon_items_property->GetValue(UIApplication::typeid));

                    dict_ribbon_panel tab_item;
                    if (ribbon_items->TryGetValue(tab->Id, tab_item)) {
                        RibbonItemCollection^ ribbon_item_collection = rib_panel->Source->Items;

                        for each (Autodesk::Windows::RibbonItem^ ri in ribbon_item_collection) {
                            if (ri->Id != nullptr && ri->Id != "") {
	                            addin_update au_btn;
                                /* button.id это строка вида CustomCtrl_%CustomCtrl_%IMJA_WKLADKI%ИмяПанели%EineKommando, парсим и панель и команду  */
                                array<string>^ tokens = ri->Id->Split('%');
                                if (tokens->Length >= 2) {                                    
                                    au_btn.command = tokens[tokens->Length - 1];
                                    au_btn.panel_name = tokens[tokens->Length - 2];
                                }
	                            au_btn.button_text = notnull(ri->Text);
	                            au_btn.image = notnull(ri->Image);
	                            au_btn.large_image = notnull(ri->LargeImage);
	                            au_btn.tool_tip = notnull(ri->ToolTip);

	                            buttons_list->Add(au_btn);
                            }
                        }
                    }
                }
                catch (exception ex) {
                    UI::TaskDialog::Show("Failed to collect buttons", ex->Message + "\n\n" + ex->StackTrace);
                }
            }
        }
    }
    return 0;
}
UI::RibbonPanel^ host_app::get_panel(const string tab_name, const string panel_name) {
    
    auto revit_panels = host_app::uic_application->GetRibbonPanels(tab_name);
    for each (auto panel in revit_panels) {
        if (panel->Name == panel_name)
            return panel;
    }
    return host_app::uic_application->CreateRibbonPanel(tab_name, panel_name);
}

void host_app::clear_panels(const string tab_name) {
    RibbonControl^ ribbon_control = ComponentManager::Ribbon;

    dict_ribbon_panel tab_item;
    RibbonTab^ bimtab = nullptr;

    auto buttons_to_remove = gcnew List<ad::RibbonItem^>;

    RibbonItemCollection^ ribbon_item_collection = nullptr;

    auto commands = host_app::commands;
    for each (RibbonTab^ tab in ribbon_control->Tabs)
    {
        if (tab->Name == tab_name) {
            bimtab = tab;
            auto ui_application_type = UIApplication::typeid;
            try {
                auto ribbon_items_property = ui_application_type->GetProperty("RibbonItemDictionary",
                    BindingFlags::NonPublic | BindingFlags::Static | BindingFlags::DeclaredOnly);

                auto ribbon_items = static_cast<Dictionary<string, dict_ribbon_panel>^>(ribbon_items_property->GetValue(UIApplication::typeid));
                
                if (ribbon_items->TryGetValue(tab->Id, tab_item)) {
                    for each (ad::RibbonPanel^ ribbon_panel in tab->Panels) {

                        ribbon_item_collection = ribbon_panel->Source->Items;
                        /* Удаляем push_buttons */

                        for each (Autodesk::Windows::RibbonItem^ ri in ribbon_item_collection) {
                            if (ri->Text != "Доставить\nобновления")
                               buttons_to_remove->Add(ri);                               
                        }

                        for (int i = 0; i < buttons_to_remove->Count; ++i)
                            ribbon_item_collection->Remove(buttons_to_remove[i]);
                    }                    
                }          
            }
            catch (exception ex) {
                UI::TaskDialog::Show("Failed to remove ribbon item", ex->Message + "\n\n" + ex->StackTrace);
            }
        }
    }
}

bool host_app::is_netpath_available(const std::wstring& path, const int timeout_ms) {

	/* Используем async с таймаутом для обхода зависаний */
    auto future = std::async(std::launch::async, [path]() -> bool {

	    /* Пробуем открыть временный файл для проверки доступа */
	    std::wstring test_path = path;
	    if (test_path.back() != L'\\' && test_path.back() != L'/')
	        test_path += L'\\';
	    test_path += L".network_test_tmp";

	    const HANDLE h_file = CreateFileW(
	        test_path.c_str(),
	        GENERIC_WRITE,
	        FILE_SHARE_READ,
	        nullptr,
	        CREATE_ALWAYS,
	        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
	        nullptr
	    );

	    if (h_file != INVALID_HANDLE_VALUE) {
	        CloseHandle(h_file);
	        return true;
	    }
	    return false;
    });

    const auto status = future.wait_for(std::chrono::milliseconds(timeout_ms));
    return (status == std::future_status::ready) && future.get();
}

static array<Byte>^ get_key() {
    /* 32 байта (AES-256) */
    auto k = gcnew array<Byte>(32);

    for (int i = 0; i < k->Length; i++) {
        k[i] = static_cast<Byte>((i * 31 + 7) ^ 0xAA);
    }
    return k;
}
static array<Byte>^ get_iv() {
    auto iv = gcnew array<Byte>(16);

    for (int i = 0; i < iv->Length; i++) {
        iv[i] = static_cast<Byte>((i * 17 + 3) ^ 0x55);
    }
    return iv;
}
string host_app::aes_decode(string obf) {
    if (String::IsNullOrEmpty(obf))
        return String::Empty;

    array<Byte>^ cipher = System::Convert::FromBase64String(obf);

    Aes^ aes = Aes::Create();
    aes->Key = get_key();
    aes->IV = get_iv();
    aes->Mode = CipherMode::CBC;
    aes->Padding = PaddingMode::PKCS7;

    auto ms = gcnew MemoryStream(cipher);
    auto cs = gcnew CryptoStream(ms, aes->CreateDecryptor(), CryptoStreamMode::Read);
    auto sr = gcnew StreamReader(cs, Encoding::UTF8, true, 4096, false);

    String^ result = sr->ReadToEnd();

    sr->Close();
    cs->Close();
    ms->Close();

    return result;
}