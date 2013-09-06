/*
 * Copyright (c) 2012 Chhatoi Pritam Baral <pritam@pritambaral.com>. All rights reserved.
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * 
 */ 

#include <gtk/gtk.h>
#include "appshell_extensions.h"
#include "appshell_extensions_platform.h"
#include "client_handler.h"
#include "native_menu_model.h"

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <X11/Xlib.h>

GtkWidget* _menuWidget;

int ConvertLinuxErrorCode(int errorCode, bool isReading = true);
int ConvertGnomeErrorCode(GError* gerror, bool isReading = true);

extern bool isReallyClosing;

static const char* GetPathToLiveBrowser() 
{
    //#TODO Use execlp and be done with it! No need to reinvent the wheel; so badly that too!
    char *envPath = getenv( "PATH" ), *path, *dir, *currentPath;

    //# copy PATH and not modify the original
    path=(char *)malloc(strlen(envPath)+1);
    strcpy(path, envPath);

    // Prepend a forward-slash. For convenience
    const char* executable="/google-chrome";
    struct stat buf;
    int len;
 
    for ( dir = strtok( path, ":" ); dir; dir = strtok( NULL, ":" ) )
    {
        len=strlen(dir)+strlen(executable);
        // if((strrchr(dir,'/')-dir)==strlen(dir))
        // {
        //     currentPath = (char*)malloc(len);
        //     strcpy(currentPath,dir);
        // } else
        // {
        // stat handles consecutive forward slashes automatically. No need for above
            currentPath = (char *)malloc(len+1);
            strncpy(currentPath,dir,len);
        //}
        strcat(currentPath,executable);
    
        if(stat(currentPath,&buf)==0 && S_ISREG(buf.st_mode))
            return currentPath;
    }

    return "";
}

int32 OpenLiveBrowser(ExtensionString argURL, bool enableRemoteDebugging)
{    
    //# COnsider using execlp and avoid all this path mess!
    const char  *appPath = GetPathToLiveBrowser(),
                *arg1 = "--allow-file-access-from-files";
    std::string arg2(" ");

    if(enableRemoteDebugging)
        arg2.assign("--remote-debugging-port=9222");

    short int error=0;
    /* Using vfork() 'coz I need a shared variable for the error passing.
     * Do not replace with fork() unless you have a better way. */
    pid_t pid = vfork();
    switch(pid)
    {
        case -1:    //# Something went wrong
                return ConvertLinuxErrorCode(errno);
        case 0:     //# I'm the child. When I successfully exec, parent is resumed. Or when I _exec()
                execl(appPath, arg1, argURL.c_str(), arg2.c_str(),(char *)0);

                error=errno;
                _exit(0);
        default:
                if(error!=0)
                {
                    printf("Error!! %s\n", strerror(error));
                    return ConvertLinuxErrorCode(error);
                }
    }

    return NO_ERROR;
}

void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    system("killall -9 google-chrome &");
}

// void CloseLiveBrowser(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
// {
//     LiveBrowserMgrLin* liveBrowserMgr = LiveBrowserMgrLin::GetInstance();
    
//     if (liveBrowserMgr->GetCloseCallback() != NULL) {
//         // We can only handle a single async callback at a time. If there is already one that hasn't fired then
//         // we kill it now and get ready for the next.
//         liveBrowserMgr->CloseLiveBrowserFireCallback(ERR_UNKNOWN);
//     }

//     liveBrowserMgr->SetCloseCallback(response);
//     liveBrowserMgr->SetBrowser(browser);

//     EnumChromeWindowsCallbackData cbData = {0};

//     cbData.numberOfFoundWindows = 0;
//     cbData.closeWindow = true;
//     ::EnumWindows(LiveBrowserMgrLin::EnumChromeWindowsCallback, (long)&cbData);

//     if (cbData.numberOfFoundWindows == 0) {
//         liveBrowserMgr->CloseLiveBrowserFireCallback(NO_ERROR);
//     } else if (liveBrowserMgr->GetCloseCallback()) {
//         // set a timeout for up to 3 minutes to close the browser 
//         liveBrowserMgr->SetCloseTimeoutTimerId( ::SetTimer(NULL, 0, 3 * 60 * 1000, LiveBrowserMgrLin::CloseLiveBrowserTimerCallback) );
//     }
// }

int32 OpenURLInDefaultBrowser(ExtensionString url)
{
    GError* error = NULL;
    gtk_show_uri(NULL, url.c_str(), GDK_CURRENT_TIME, &error);
    g_error_free(error);
    return NO_ERROR;
}

int32 IsNetworkDrive(ExtensionString path, bool& isRemote)
{
}

int32 ShowOpenDialog(bool allowMultipleSelection,
                     bool chooseDirectory,
                     ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString fileTypes,
                     CefRefPtr<CefListValue>& selectedFiles)
{
    GtkWidget *dialog;
    const char* dialog_title = chooseDirectory ? "Open Directory" : "Open File";
    GtkFileChooserAction file_or_directory = chooseDirectory ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN ;
    dialog = gtk_file_chooser_dialog_new (dialog_title,
                  NULL,
                  file_or_directory,
                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                  NULL);
    
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        selectedFiles->SetString(0,filename);
        g_free (filename);
    }
    
    gtk_widget_destroy (dialog);
    return NO_ERROR;
}

int32 ShowSaveDialog(ExtensionString title,
                     ExtensionString initialDirectory,
                     ExtensionString proposedNewFilename,
                     ExtensionString& newFilePath)
{
    GtkWidget *openSaveDialog;
    
    openSaveDialog = gtk_file_chooser_dialog_new(title.c_str(),
        NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (openSaveDialog), TRUE);	
    if (!initialDirectory.empty())
    {
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (openSaveDialog), proposedNewFilename.c_str());
        
        ExtensionString folderURI = std::string("file:///") + initialDirectory;
        gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (openSaveDialog), folderURI.c_str());
    }
    
    if (gtk_dialog_run (GTK_DIALOG (openSaveDialog)) == GTK_RESPONSE_ACCEPT)
    {
        char* filePath;
        filePath = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (openSaveDialog));
        newFilePath = filePath;
        g_free (filePath);
    }
    
    gtk_widget_destroy (openSaveDialog);
    return NO_ERROR;
}

int32 ReadDir(ExtensionString path, CefRefPtr<CefListValue>& directoryContents)
{
    //# Add trailing /slash if neccessary
    if (path.length() && path[path.length() - 1] != '/')
        path += '/';

    DIR *dp;
    struct dirent *files;
    
    /*struct dirent
    {
        unsigned char  d_type; #not supported on all systems, faster than stat
        char d_name[]
    }*/
    

    if((dp=opendir(path.c_str()))==NULL)
        return ConvertLinuxErrorCode(errno,true);

    std::vector<ExtensionString> resultFiles;
    std::vector<ExtensionString> resultDirs;

    while((files=readdir(dp))!=NULL)
    {
        if(!strcmp(files->d_name,".") || !strcmp(files->d_name,".."))
            continue;
        if(files->d_type==DT_DIR)
            resultDirs.push_back(ExtensionString(files->d_name));
        else if(files->d_type==DT_REG)
            resultFiles.push_back(ExtensionString(files->d_name));
    }

    closedir(dp);

    //# List dirs first, files next
    size_t i, total = 0;
    for (i = 0; i < resultDirs.size(); i++)
        directoryContents->SetString(total++, resultDirs[i]);
    for (i = 0; i < resultFiles.size(); i++)
        directoryContents->SetString(total++, resultFiles[i]);

    return NO_ERROR;
}

int32 MakeDir(ExtensionString path, int mode)
{
    static int mkdirError = NO_ERROR;
    mode = mode | 0777;     //#TODO make Brackets set mode != 0

    struct stat buf;
    if((stat(path.substr(0, path.find_last_of('/')).c_str(), &buf) < 0) && errno==ENOENT)
        MakeDir(path.substr(0, path.find_last_of('/')), mode);

    if(mkdirError != 0)
        return mkdirError;

    if(mkdir(path.c_str(),mode)==-1)
        mkdirError = ConvertLinuxErrorCode(errno);
    return NO_ERROR;
}

int Rename(ExtensionString oldName, ExtensionString newName)
{
    if (rename(oldName.c_str(), newName.c_str())==-1)
        return ConvertLinuxErrorCode(errno);
}

int GetFileModificationTime(ExtensionString filename, uint32& modtime, bool& isDir)
{
    struct stat buf;
    if(stat(filename.c_str(),&buf)==-1)
        return ConvertLinuxErrorCode(errno);

    modtime = buf.st_mtime;
    isDir = S_ISDIR(buf.st_mode);

    return NO_ERROR;
}

int ReadFile(ExtensionString filename, ExtensionString encoding, std::string& contents)
{

    if(encoding != "utf8")
        return NO_ERROR;    //#TODO ERR_UNSUPPORTED_ENCODING

    struct stat buf;
    if(stat(filename.c_str(),&buf)==-1)
        return ConvertLinuxErrorCode(errno);

    if(!S_ISREG(buf.st_mode))
        return NO_ERROR;    //# TODO ERR_CANT_READ when cleaninp up errors

    FILE* file = fopen(filename.c_str(),"r");
    if(file == NULL)
    {
        return ConvertLinuxErrorCode(errno);
    }

    fseek(file, 0, SEEK_END);
    long int size = ftell(file);
    rewind(file);

    char* content = (char*)calloc(size + 1, 1);

    fread(content,1,size,file);
    if(fclose(file)==EOF)
        return ConvertLinuxErrorCode(errno);

    contents = content;

    return NO_ERROR;
}

int32 WriteFile(ExtensionString filename, std::string contents, ExtensionString encoding)
{
    if(encoding != "utf8")
        return NO_ERROR;    //# TODO ERR_UNSUPPORTED_ENCODING;

    const char* content = contents.c_str();

    FILE* file = fopen(filename.c_str(),"w");
    if(file == NULL)
        return ConvertLinuxErrorCode(errno);

    long int size = strlen(content);

    fwrite(content,1,size,file);

    if(fclose(file)==EOF)
        return ConvertLinuxErrorCode(errno);

    return NO_ERROR;
}

int SetPosixPermissions(ExtensionString filename, int32 mode)
{
    if(chmod(filename.c_str(),mode)==-1)
        return ConvertLinuxErrorCode(errno);

    return NO_ERROR;
}

int DeleteFileOrDirectory(ExtensionString filename)
{
    if(unlink(filename.c_str())==-1)
        return ConvertLinuxErrorCode(errno);
    return NO_ERROR;
}



void MoveFileOrDirectoryToTrash(ExtensionString filename, CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> response)
{
    int error = NO_ERROR;
    GFile *file = g_file_new_for_path(filename.c_str());
    GError *gerror = NULL;
    if (!g_file_trash(file, NULL, &gerror)) {
        error = ConvertGnomeErrorCode(gerror);
        g_error_free(gerror);
    }
    g_object_unref(file);
    
    response->GetArgumentList()->SetInt(1, error);
    browser->SendProcessMessage(PID_RENDERER, response);
}

void CloseWindow(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        isReallyClosing = true;
        // //# Hack because CEF's CloseBrowser() is bad. Should emit delete_event instead of directly destroying widget
        // GtkWidget* hwnd = gtk_widget_get_toplevel (browser->GetHost()->GetWindowHandle() );
        // if(gtk_widget_is_toplevel (hwnd))
        //     gtk_signal_emit_by_name(GTK_OBJECT(hwnd), "delete_event");
        // else
            browser->GetHost()->CloseBrowser(true);
    }
}

void BringBrowserWindowToFront(CefRefPtr<CefBrowser> browser)
{
    if (browser.get()) {
        GtkWindow* hwnd = GTK_WINDOW(browser->GetHost()->GetWindowHandle());
        if (hwnd) 
            gtk_window_present(hwnd);
    }
}

int ShowFolderInOSWindow(ExtensionString pathname)
{
    GError *error;
    gtk_show_uri(NULL, pathname.c_str(), GDK_CURRENT_TIME, &error);
    
    if(error != NULL)
        g_warning ("%s %s", "Error launching preview", error->message);
    
    g_error_free(error);

    return NO_ERROR;
}


int ConvertGnomeErrorCode(GError* gerror, bool isReading) 
{
    if (gerror == NULL) 
        return NO_ERROR;

    if (gerror->domain == G_FILE_ERROR) {
        switch(gerror->code) {
        case G_FILE_ERROR_EXIST:
            return ERR_FILE_EXISTS;
        case G_FILE_ERROR_NOTDIR:
            return ERR_NOT_DIRECTORY;
        case G_FILE_ERROR_ISDIR:
            return ERR_NOT_FILE;
        case G_FILE_ERROR_NXIO:
        case G_FILE_ERROR_NOENT:
            return ERR_NOT_FOUND;
        case G_FILE_ERROR_NOSPC:
            return ERR_OUT_OF_SPACE;
        case G_FILE_ERROR_INVAL:
            return ERR_INVALID_PARAMS;
        case G_FILE_ERROR_ROFS:
            return ERR_CANT_WRITE;
        case G_FILE_ERROR_BADF:
        case G_FILE_ERROR_ACCES:
        case G_FILE_ERROR_PERM:
        case G_FILE_ERROR_IO:
           return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
        default:
            return ERR_UNKNOWN;
        }  
    } else if (gerror->domain == G_IO_ERROR) {
        switch(gerror->code) {
        case G_IO_ERROR_NOT_FOUND:
            return ERR_NOT_FOUND;
        case G_IO_ERROR_NO_SPACE:
            return ERR_OUT_OF_SPACE;
        case G_IO_ERROR_INVALID_ARGUMENT:
            return ERR_INVALID_PARAMS;
        default:
            return ERR_UNKNOWN;
        }
    }
}

int ConvertLinuxErrorCode(int errorCode, bool isReading)
{
//    printf("LINUX ERROR! %d %s\n", errorCode, strerror(errorCode));
    switch (errorCode) {
    case NO_ERROR:
        return NO_ERROR;
    case ENOENT:
        return ERR_NOT_FOUND;
    case EACCES:
        return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
    case ENOTDIR:
        return ERR_NOT_DIRECTORY;
//    case ERROR_WRITE_PROTECT:
//        return ERR_CANT_WRITE;
//    case ERROR_HANDLE_DISK_FULL:
//        return ERR_OUT_OF_SPACE;
//    case ERROR_ALREADY_EXISTS:
//        return ERR_FILE_EXISTS;
    default:
        return ERR_UNKNOWN;
    }
}

int32 CopyFile(ExtensionString src, ExtensionString dest)
{
    int error = NO_ERROR;
    GFile *source = g_file_new_for_path(src.c_str());
    GFile *destination = g_file_new_for_path(dest.c_str());
    GError *gerror = NULL;

    if (!g_file_copy(source, destination, (GFileCopyFlags)(G_FILE_COPY_OVERWRITE|G_FILE_COPY_NOFOLLOW_SYMLINKS|G_FILE_COPY_TARGET_DEFAULT_PERMS), NULL, NULL, NULL, &gerror)) {
        error = ConvertGnomeErrorCode(gerror);
        g_error_free(gerror);
    }
    g_object_unref(source);
    g_object_unref(destination);

    return error;    
}

int32 GetPendingFilesToOpen(ExtensionString& files)
{
}

GtkWidget* GetMenu(CefRefPtr<CefBrowser> browser)
{
    GtkWidget* window = (GtkWidget*)getMenuParent(browser);
    GtkWidget* widget;
    GList *children, *iter, *iter2;

    children = gtk_container_get_children(GTK_CONTAINER(window));
    
    for(iter = children; iter != NULL; iter = g_list_next(iter)) {
        widget = (GtkWidget*)iter->data;

        if (GTK_IS_VBOX(widget)) {
            GList* vboxChildren = gtk_container_get_children(GTK_CONTAINER(widget));
//            g_message("%d", g_list_length(vboxChildren));
            
            for(iter2 = vboxChildren; iter2 != NULL; iter2 = g_list_next(iter2)) {
                GtkWidget* menu = (GtkWidget*)iter2->data;
                if(GTK_IS_MENU_BAR(menu)) {
                    g_message("Found the menubar");
                    return menu;
                }
            }
        };
    }

    return NULL;
}


GtkWidget* AddMenuEntry(GtkWidget* menu_widget, const char* text,
                        GCallback callback) {
  GtkWidget* entry = gtk_menu_item_new_with_label(text);
  g_signal_connect(entry, "activate", callback, NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_widget), entry);
  return entry;
}

GtkWidget* _CreateMenu(GtkWidget* menu_bar, const char* text) {
  GtkWidget* menu_widget = gtk_menu_new();
  GtkWidget* menu_header = gtk_menu_item_new_with_label(text);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_header), menu_widget);
//  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_header);
  return menu_widget;
}

// Callback for Debug > Get Source... menu item.
gboolean GetSourceActivated(GtkWidget* widget) {
  return FALSE;
}

// Return index where menu or menu item should be placed.
// -1 indicates append. -2 indicates 'before' - WINAPI supports
// placing a menu before an item without needing the position.

// TODO: I need to find a way to attach the tag from the NativeMenuModel to the
// gtk Menu/MenuItem like we do this for Windows and Mac
// keep track of mapping from gtk widget to NativeMenuModel tag
typedef std::map<int, const void*> tagToMenuWidgetMapping;
tagToMenuWidgetMapping tag2Widget;

const void* findWidgetForTag(const int tag) {
    tagToMenuWidgetMapping::iterator  it = tag2Widget.find(tag);
    
    if (it != tag2Widget.end()) {
        return it->second;
    }
    
    return NULL;
}

void mapTagToWidget(const int tag, const void* widget) {
    tagToMenuWidgetMapping::iterator foundItem = tag2Widget.find(tag);
    
    if(foundItem == tag2Widget.end()) {
        tag2Widget[tag] = widget;
    }
}

const int kAppend = -1;
const int kBefore = -2;

int32 GetMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId, ExtensionString& parentId, int& index)
{
    g_message("GetMenuPosition: commandId=%s, parentId=%s", commandId.c_str(), parentId.c_str());

    index = -1;
    parentId = ExtensionString();
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(commandId);
    
    if (tag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }

    g_message("Found tag %d for commandId (%s)", tag, commandId.c_str());

    GtkWidget* parentMenu = (GtkWidget*) NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(tag);
    g_message("Found Objecttype '%s'", GTK_OBJECT_TYPE_NAME(parentMenu));
    if (parentMenu == NULL) {
//        parentMenu = GetMenu((HWND)getMenuParent(browser));
    } else {
        parentId = NativeMenuModel::getInstance(getMenuParent(browser)).getParentId(tag);
    }

    GList *menuItems = gtk_container_get_children(GTK_CONTAINER(GetMenu(browser)));
    guint elements = g_list_length(menuItems);
    guint i;

    const void* targetWidget = findWidgetForTag(tag);
    if (targetWidget) {
        g_message("Found target widget");
        g_message("=> Found Objecttype %s", GTK_OBJECT_TYPE_NAME(targetWidget));
    }

    for(i = 0; i < elements; i++) {
        GtkWidget* menuItem = (GtkWidget*)g_list_nth_data(menuItems, i);
        g_message("=> Found Objecttype %s with label %s", GTK_OBJECT_TYPE_NAME(menuItem), gtk_menu_item_get_label(GTK_MENU_ITEM(menuItem)));
        
        if (menuItem == targetWidget) {
            index = i;
            g_message("Found at position %d", index);
            return NO_ERROR;
        }
    }        

    return ERR_NOT_FOUND;
}

GtkWidget* getMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& id)
{
    NativeMenuModel model = NativeMenuModel::getInstance(getMenuParent(browser));
    int32 tag = model.getTag(id);

//     MENUITEMINFO parentItemInfo = {0};
//     parentItemInfo.cbSize = sizeof(MENUITEMINFO);
//     parentItemInfo.fMask = MIIM_SUBMENU;
//     if (!GetMenuItemInfo((HMENU)model.getOsItem(tag), tag, FALSE, &parentItemInfo)) {
//         return 0;
//     }
//     return parentItemInfo.hSubMenu;
}

int32 getNewMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& parentId, const ExtensionString& position, const ExtensionString& relativeId, int32& positionIdx)
{
    g_message("getNewMenuPosition: parentId (%s), position (%s), relativeId (%s), positionIdx (%d)", parentId.c_str(), position.c_str(), relativeId.c_str(), positionIdx);

    int32 errCode = NO_ERROR;
    ExtensionString pos = position;
    ExtensionString relId = relativeId;
    NativeMenuModel model = NativeMenuModel::getInstance(getMenuParent(browser));

    if (pos.size() == 0)
    {
        positionIdx = kAppend;
    } else if (pos == "first") {
        positionIdx = 0;
    } else if (pos == "firstInSection" || pos == "lastInSection") {
        int32 startTag = model.getTag(relId);
        GtkWidget* startItem = (GtkWidget*) model.getOsItem(startTag);
        ExtensionString startParentId = model.getParentId(startTag);
        GtkWidget* parentMenu = (GtkWidget*) model.getOsItem(model.getTag(startParentId));
        int32 idx;

        if (startParentId != parentId) {
            // Section is in a different menu
            positionIdx = -1;
            return ERR_NOT_FOUND;
        }

//         parentMenu = getMenu(browser, startParentId);
//
//         GetMenuPosition(browser, relId, startParentId, idx);
//
//         if (pos == L"firstInSection") {
//             // Move backwards until reaching the beginning of the menu or a separator
//             while (idx >= 0) {
//                 if (isSeparator(parentMenu, idx)) {
//                     break;
//                 }
//                 idx--;
//             }
//             if (idx < 0) {
//                 positionIdx = 0;
//             } else {
//                 idx++;
//                 pos = L"before";
//             }
//         } else { // "lastInSection"
//             int32 numItems = GetMenuItemCount(parentMenu);
//             // Move forwards until reaching the end of the menu or a separator
//             while (idx < numItems) {
//                 if (isSeparator(parentMenu, idx)) {
//                     break;
//                 }
//                 idx++;
//             }
//             if (idx == numItems) {
//                 positionIdx = -1;
//             } else {
//                 idx--;
//                 pos = L"after";
//             }
//         }
//
//         if (pos == L"before" || pos == L"after") {
//             MENUITEMINFO itemInfo = {0};
//             itemInfo.cbSize = sizeof(MENUITEMINFO);
//             itemInfo.fMask = MIIM_ID;
//
//             if (!GetMenuItemInfo(parentMenu, idx, TRUE, &itemInfo)) {
//                 int err = GetLastError();
//                 return ConvertErrnoCode(err);
//             }
//             relId = model.getCommandId(itemInfo.wID);
//         }
    }

    if ((pos == "before" || pos == "after") && relId.size() > 0) {
        if (pos == "before") {
            positionIdx = kBefore;
        } else {
            positionIdx = kAppend;
        }

        ExtensionString newParentId;
        errCode = GetMenuPosition(browser, relId, newParentId, positionIdx);

        if (parentId.size() > 0 && parentId != newParentId) {
            errCode = ERR_NOT_FOUND;
        }

        // If we don't find the relative ID, return an error and
        // set positionIdx to kAppend. The item will be appended and an
        // error will be shown in the console.
        if (errCode == ERR_NOT_FOUND) {
            positionIdx = kAppend;
        } else if (positionIdx >= 0 && pos == "after") {
            positionIdx += 1;
        }
     }

    return errCode;
}

int32 AddMenu(CefRefPtr<CefBrowser> browser, ExtensionString title, ExtensionString command,
              ExtensionString position, ExtensionString relativeId)
{
    g_message("AddMenu: title (%s), command (%s), position (%s), relativeId (%s)", title.c_str(), command.c_str(), position.c_str(), relativeId.c_str());

    GtkWidget* mainMenu = NULL;
    GtkWidget* menuLabel = NULL;
    GtkWidget* menuBar = NULL;
    
    int32 tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound) {
        g_message("No tag found for %s. Create new tag", command.c_str());
        tag = NativeMenuModel::getInstance(getMenuParent(browser)).getOrCreateTag(command, ExtensionString());

        menuBar = GetMenu(browser);
        
        mainMenu = gtk_menu_new();
        menuLabel = gtk_menu_item_new_with_label(title.c_str());
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuLabel), mainMenu);
        
        NativeMenuModel::getInstance(getMenuParent(browser)).setOsItem(tag, (void*)mainMenu);

        // update our model
        // this is not good! the NativeMenuModel maps tag => mainMenu and I
        // need to map tag => menuLabel to get my hand to the menu... There must
        // be a better way
        mapTagToWidget(tag, menuLabel);
    } else {
        // menu already there
        return NO_ERROR;
    }

    bool inserted = false;
    int32 positionIdx;
    int32 errCode = getNewMenuPosition(browser, "", position, relativeId, positionIdx);

    g_message("New Menu Position: (%d)", positionIdx);

    if (positionIdx >= 0 || positionIdx == kBefore)
    {
        if (positionIdx >= 0) {
            gtk_menu_shell_insert(GTK_MENU_SHELL(menuBar), menuLabel, positionIdx);

            inserted = true;
        }
        else
        {
            int32 relativeTag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(relativeId);
            if (relativeTag >= 0 && positionIdx == kBefore) {
//                 InsertMenuItem(mainMenu, relativeTag, FALSE, &menuInfo);
                inserted = true;
            } else {
                // menu is already there
                return NO_ERROR;
            }
        }
    }

    if (inserted == false) {
        gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), menuLabel);
//        g_message("Inserted new Menu");
    }

    gtk_widget_show_all(menuBar);

    return errCode;
}

void FakeCallback() {
}

int32 AddMenuItem(CefRefPtr<CefBrowser> browser, ExtensionString parentCommand, ExtensionString itemTitle,
                  ExtensionString command, ExtensionString key, ExtensionString displayStr,
                  ExtensionString position, ExtensionString relativeId)
{
    g_message("AddMenuItem: parentCommand (%s), itemTitle (%s), command (%s), key (%s), displayStr (%s), position (%s), relativeId (%s)", parentCommand.c_str(), itemTitle.c_str(), command.c_str(), key.c_str(), displayStr.c_str(), position.c_str(), relativeId.c_str());

    int32 parentTag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(parentCommand);
    if (parentTag == kTagNotFound) {
        return ERR_NOT_FOUND;
    }

    GtkWidget* menu = (GtkWidget* )NativeMenuModel::getInstance(getMenuParent(browser)).getOsItem(parentTag);
    if (menu == NULL)
        return ERR_NOT_FOUND;

    bool isSeparator = (itemTitle == "---");
    GtkWidget* submenu = NULL;
    if (isSeparator) {
        submenu = gtk_separator_menu_item_new();
    } else {
        submenu = gtk_menu_item_new_with_label(itemTitle.c_str());
        g_signal_connect(submenu, "activate", FakeCallback, NULL);
    }
//     return NO_ERROR;

//     HMENU submenu = NULL;
//     MENUITEMINFO parentItemInfo;
//
//     memset(&parentItemInfo, 0, sizeof(MENUITEMINFO));
//     parentItemInfo.cbSize = sizeof(MENUITEMINFO);
//     parentItemInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_SUBMENU | MIIM_STRING;
//     //get the menuitem containing this one, see if it has a sub menu. If it doesn't, add one.
//     if (!GetMenuItemInfo(menu, parentTag, FALSE, &parentItemInfo)) {
//         return ConvertErrnoCode(GetLastError());
//     }
//     if (parentItemInfo.hSubMenu == NULL) {
//         parentItemInfo.hSubMenu = CreateMenu();
//         parentItemInfo.fMask = MIIM_SUBMENU;
//         if (!SetMenuItemInfo(menu, parentTag, FALSE, &parentItemInfo)) {
//             return ConvertErrnoCode(GetLastError());
//         }
//     }
//     submenu = parentItemInfo.hSubMenu;
    int32 tag = -1;
    ExtensionString title;
    ExtensionString keyStr;
    ExtensionString displayKeyStr;

    tag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(command);
    if (tag == kTagNotFound) {
        tag = NativeMenuModel::getInstance(getMenuParent(browser)).getOrCreateTag(command, parentCommand);
    } else {
        return NO_ERROR;
    }

    title = itemTitle.c_str();
    keyStr = key.c_str();

    if (key.length() > 0 && displayStr.length() == 0) {
//         displayKeyStr = GetDisplayKeyString(keyStr);
    } else {
        displayKeyStr = displayStr;
    }

    if (displayKeyStr.length() > 0) {
        title = title + "\t" + displayKeyStr;
    }
    int32 positionIdx;
    int32 errCode = getNewMenuPosition(browser, parentCommand, position, relativeId, positionIdx);
    bool inserted = false;
//     if (positionIdx >= 0 || positionIdx == kBefore)
//     {
//         MENUITEMINFO menuInfo;
//         memset(&menuInfo, 0, sizeof(MENUITEMINFO));
//         menuInfo.cbSize = sizeof(MENUITEMINFO);
//         menuInfo.wID = (UINT)tag;
//         menuInfo.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE;
//         menuInfo.fType = MFT_STRING;
//         if (isSeparator) {
//             menuInfo.fType = MFT_SEPARATOR;
//         }
//         menuInfo.dwTypeData = (LPWSTR)title.c_str();
//         menuInfo.cch = itemTitle.size();
//         if (positionIdx >= 0) {
//             InsertMenuItem(submenu, positionIdx, TRUE, &menuInfo);
//             inserted = true;
//         }
//         else
//         {
//             int32 relativeTag = NativeMenuModel::getInstance(getMenuParent(browser)).getTag(relativeId);
//             if (relativeTag >= 0 && positionIdx == kBefore) {
//                 InsertMenuItem(submenu, relativeTag, FALSE, &menuInfo);
//                 inserted = true;
//             } else {
//                 // menu is already there
//                 return NO_ERROR;
//             }
//         }
//     }

//     if (!inserted)
//     {
//         BOOL res;
//
//         if (isSeparator)
//         {
//             res = AppendMenu(submenu, MF_SEPARATOR, NULL, NULL);
//         } else {
//             res = AppendMenu(submenu, MF_STRING, tag, title.c_str());
//         }
//
//         if (!res) {
//             return ConvertErrnoCode(GetLastError());
//         }
//     }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), submenu);

    NativeMenuModel::getInstance(getMenuParent(browser)).setOsItem(tag, (void*)submenu);
//     DrawMenuBar((HWND)getMenuParent(browser));

//     if (!isSeparator && !UpdateAcceleratorTable(tag, keyStr)) {
//         title = title.substr(0, title.find('\t'));
//         SetMenuTitle(browser, command, title);
//     }

    return errCode;
}

int32 RemoveMenu(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    return NO_ERROR;
}

int32 RemoveMenuItem(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId)
{
    return NO_ERROR;
}

int32 GetMenuItemState(CefRefPtr<CefBrowser> browser, ExtensionString commandId, bool& enabled, bool& checked, int& index)
{
    return NO_ERROR;
}

int32 SetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString menuTitle)
{
    return NO_ERROR;
}

int32 GetMenuTitle(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString& menuTitle)
{
    return NO_ERROR;
}

int32 SetMenuItemShortcut(CefRefPtr<CefBrowser> browser, ExtensionString commandId, ExtensionString shortcut, ExtensionString displayStr)
{
    return NO_ERROR;
}

// int32 GetMenuPosition(CefRefPtr<CefBrowser> browser, const ExtensionString& commandId, ExtensionString& parentId, int& index)
// {
//     return NO_ERROR;
// }

void DragWindow(CefRefPtr<CefBrowser> browser)
{
    // TODO
}

