#pragma once

#include "stdafx.h"
#include "isolated_load_context.h"

namespace bi_loader {
	
    [Transaction(TransactionMode::Manual)]
	/* Класс-обработчик нажатия кнопки запуска ifc exporter.exe */
    public ref class btn_click sealed : IExternalCommand {
    public:
        virtual Result Execute(ExternalCommandData^ command_data, string% message, ElementSet^ elements);
    };
	
	public ref class btn_availability : IExternalCommandAvailability {
    public:
    	virtual bool IsCommandAvailable(UIApplication^ uiapp, CategorySet^ cat_set);
    };
};