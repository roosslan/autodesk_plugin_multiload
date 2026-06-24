#pragma once

#include "stdafx.h"

namespace bi_loader {
	public ref class wrapper_into_context : IExternalEventHandler {
	public:
		wrapper_into_context(UIControlledApplication^ addin_uicap);
		IExternalApplication^ addin_;
		UIControlledApplication^ uic_app_addin_;
		void set_iexternal_app(IExternalApplication^ addin_app);
		void task_run_async_in_context(UIApplication^ app);

		/* Унаследовано через IExternalEventHandler */
		virtual string GetName();	
		virtual void Execute(UIApplication^ app);		
	};
}