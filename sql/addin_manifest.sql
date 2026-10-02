/*
 *  Манифест SHA-256 для проверки файлов обновления и учётная запись только на чтение.
 *  Выполняется администратором БД, пароль подставляется вручную.
 */

USE [bi];
GO

/* Манифест: по одной строке на каждый .dll/.exe из директории обновления */
IF OBJECT_ID(N'[dbo].[addin_manifest]', N'U') IS NULL
    CREATE TABLE [dbo].[addin_manifest] (
        [relative_path] nvarchar(512) NOT NULL PRIMARY KEY, /* относительно директории обновления, например revit_addin26.dll */
        [sha256]        char(64)      NOT NULL              /* шестнадцатеричная строка, регистр не важен */
    );
GO

/* Учётная запись плагина: только SELECT на две таблицы */
CREATE LOGIN [bi_loader_reader] WITH PASSWORD = N'<ПАРОЛЬ>', CHECK_POLICY = ON;
GO
CREATE USER [bi_loader_reader] FOR LOGIN [bi_loader_reader];
GO
GRANT SELECT ON [dbo].[addin_updates]  TO [bi_loader_reader];
GRANT SELECT ON [dbo].[addin_manifest] TO [bi_loader_reader];
GO
