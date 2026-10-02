#include "stdafx.h"
#include "host_app.h"

using namespace bi_loader;

/* если nullptr, то возвращаем "" */
template<typename T>
string notnull(T value) {
    return value != nullptr ? value->ToString() : String::Empty;
}

/* Строка подключения к БД. Учётная запись в sensitive_data.h должна иметь только SELECT на addin_updates и addin_manifest (см. sql/addin_manifest.sql) */
static string build_connection_string() {
    auto builder = gcnew SqlConnectionStringBuilder();
    builder->DataSource = gcnew String(wdb_hostname);
    builder->InitialCatalog = gcnew String(wdb_name);
    builder->UserID = gcnew String(wdb_user);
    builder->Password = gcnew String(wdb_passwd);
    builder->ConnectTimeout = 5;
    return builder->ConnectionString;
}

array<addin_update>^ host_app::get_version_from_db()
try {
    auto updates_list = gcnew List<addin_update>();

    auto conn = gcnew SqlConnection(build_connection_string());
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

Dictionary<string, string>^ host_app::get_manifest_from_db(string% error)
try {
    auto manifest = gcnew Dictionary<string, string>(StringComparer::OrdinalIgnoreCase);

    auto conn = gcnew SqlConnection(build_connection_string());
    try {
        conn->Open();
        auto sqlcmd = gcnew SqlCommand("select relative_path, sha256 from addin_manifest", conn);
        auto reader = sqlcmd->ExecuteReader();
        try {
            while (reader->Read()) {
                /* пути в манифесте нормализуем к виду с обратной косой чертой */
                const string relative_path = reader["relative_path"]->ToString()->Replace('/', '\\')->Trim();
                manifest[relative_path] = reader["sha256"]->ToString()->Trim();
            }
        }
        finally {
            reader->Close();
        }
    }
    finally {
        conn->Close();
    }
    return manifest;
}
catch (exception ex) {
    error = ex->Message;
    return nullptr;
}

/* SHA-256 файла в виде строки из 64 шестнадцатеричных символов */
static string compute_sha256(string file_path) {
    auto sha = SHA256::Create();
    auto stream = File::OpenRead(file_path);
    try {
        return BitConverter::ToString(sha->ComputeHash(stream))->Replace("-", "");
    }
    finally {
        delete stream;
        delete sha;
    }
}

bool host_app::verify_manifest(string directory, Dictionary<string, string>^ manifest, string% error) {
    if (manifest == nullptr || manifest->Count == 0) {
        error = "Манифест SHA-256 в БД пуст или недоступен";
        return false;
    }

    const string root = Path::GetFullPath(directory)->TrimEnd('\\') + "\\";

    /* Каждый файл из манифеста должен присутствовать и совпадать по хешу */
    for each (KeyValuePair<string, string> entry in manifest) {
        const string full_path = Path::GetFullPath(Path::Combine(root, entry.Key));
        if (!full_path->StartsWith(root, StringComparison::OrdinalIgnoreCase)) {
            error = "Недопустимый путь в манифесте: " + entry.Key;
            return false;
        }
        if (!File::Exists(full_path)) {
            error = "Файл из манифеста не найден: " + entry.Key;
            return false;
        }
        if (!String::Equals(compute_sha256(full_path), entry.Value, StringComparison::OrdinalIgnoreCase)) {
            error = "Хеш SHA-256 не совпадает с манифестом: " + entry.Key;
            return false;
        }
    }

    /* Любой исполняемый файл, которого нет в манифесте, считается посторонним */
    for each (string file in Directory::GetFiles(root, "*", SearchOption::AllDirectories)) {
        const string ext = Path::GetExtension(file);
        if (!ext->Equals(".dll", StringComparison::OrdinalIgnoreCase) && !ext->Equals(".exe", StringComparison::OrdinalIgnoreCase))
            continue;
        const string relative_path = file->Substring(root->Length);
        if (!manifest->ContainsKey(relative_path)) {
            error = "Исполняемый файл отсутствует в манифесте: " + relative_path;
            return false;
        }
    }
    return true;
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

/* Проверка доступности сетевого пути в фоновом потоке: недоступная шара не должна подвешивать Revit */
ref class net_path_probe {
    string path_;
public:
    bool available;

    net_path_probe(string path) : path_(path), available(false) {}

    void run() {
        try {
            available = Directory::Exists(path_);
        }
        catch (exception) {
            available = false;
        }
    }
};

bool host_app::is_netpath_available(const std::wstring& path, const int timeout_ms) {
    if (path.empty())
        return false;

    auto probe = gcnew net_path_probe(gcnew String(path.c_str()));
    auto thread = gcnew Thread(gcnew ThreadStart(probe, &net_path_probe::run));
    /* фоновый поток не мешает закрытию Revit, даже если запрос к шаре так и не вернулся */
    thread->IsBackground = true;
    thread->Start();

    if (!thread->Join(timeout_ms))
        return false;
    return probe->available;
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