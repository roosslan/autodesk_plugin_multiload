SELECT TOP (1000) [button_id] int
>,[button_text]	nvarchar(MAX) \
      ,[command]	nvarchar(MAX) \
      ,[large_image]	nvarchar(MAX) \
      ,[image]	nvarchar(MAX) \
      ,[tool_tip]	nvarchar(MAX) \
      ,[panel_name]	nvarchar(MAX) \
      ,[version]	nvarchar(MAX) \
      ,[button_type]	nvarchar(MAX) \
      ,[parent_button]	int
      
FROM [bi].[dbo].[addin_updates] \
\
![sql](https://github.com/roosslan/autodesk_plugin_unload/blob/trunk/sql-table.gif)
