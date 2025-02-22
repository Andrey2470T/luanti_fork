// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIElement.h"
#include "FilesystemVersions.h"

//! Standard file chooser dialog.
/** \warning When the user selects a folder this does change the current working directory

\par This element can create the following events of type EGUI_EVENT_TYPE:
\li EGET_DIRECTORY_SELECTED
\li EGET_FILE_SELECTED
\li EGET_FILE_CHOOSE_DIALOG_CANCELLED
*/
class IGUIFileOpenDialog : public IGUIElement
{
public:
	//! constructor
    IGUIFileOpenDialog(IGUIEnvironment *environment, std::shared_ptr<IGUIElement> parent, s32 id, recti rectangle) :
            IGUIElement(GUIElementType::FileOpenDialog, environment, parent, id, rectangle) {}

	//! Returns the filename of the selected file converted to wide characters. Returns NULL if no file was selected.
	virtual const wchar_t *getFileName() const = 0;

	//! Returns the filename of the selected file. Is empty if no file was selected.
    virtual const fs::path &getFileNameP() const = 0;

	//! Returns the directory of the selected file. Empty if no directory was selected.
    virtual const fs::path &getDirectoryName() const = 0;

	//! Returns the directory of the selected file converted to wide characters. Returns NULL if no directory was selected.
	virtual const wchar_t *getDirectoryNameW() const = 0;
};
