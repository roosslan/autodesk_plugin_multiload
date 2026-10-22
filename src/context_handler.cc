#include "stdafx.h"

#include "context_handler.h"

namespace bi_loader {
    wrapper_into_context::wrapper_into_context(UIControlledApplication^ addin_uicap) {
        uic_app_addin_ = addin_uicap;
    }

    void wrapper_into_context::Execute(UIApplication^ app)
    try {
        /*
        Document^ doc = app->ActiveUIDocument->Document;
        UIDocument^ uiDoc = app->ActiveUIDocument;

        if (doc == nullptr) {
            TaskDialog::Show("External Event wrapper_into_context", "Не удалось получить активный документ Revit!");
            return;
        }
        */

        task_run_async_in_context(app);
    }
    catch (const std::exception& e) {
        (void)e;
        /* File::AppendAllText("\\bi_loader.dev.log", DateTime::Now.ToString("dd.MM.yyyy hh:mm tt") + "wrapper_into_context::Execute ");  */
    }

    void wrapper_into_context::task_run_async_in_context(UIApplication^ app) {
        addin_->OnStartup(uic_app_addin_);
    }

    void wrapper_into_context::set_iexternal_app(IExternalApplication^ addin_app) {
        addin_ = addin_app;
    }

    /* virtual */
    string wrapper_into_context::GetName() {
        /* throw gcnew System::NotImplementedException();   */
        return ("External Event wrapper_into_context");
    }

}

