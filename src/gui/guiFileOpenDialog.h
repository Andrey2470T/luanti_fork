// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IGUIFileOpenDialog.h"
#include "IGUIButton.h"
#include "IGUIListBox.h"
#include "IGUIEditBox.h"
#include "IGUIEnvironment.h"
#include <Core/Events.h>
#include <FilesystemVersions.h>

class UISpriteBank;

namespace gui
{

class CGUIFileOpenDialog : public IGUIFileOpenDialog
{
public:
	//! constructor
	CGUIFileOpenDialog(const wchar_t *title, IGUIEnvironment *environment,
			IGUIElement *parent, s32 id, bool restoreCWD = false,
            std::string *startDir = 0);

	//! destructor
	virtual ~CGUIFileOpenDialog();

	//! returns the filename of the selected file. Returns NULL, if no file was selected.
	const wchar_t *getFileName() const override;

	//! Returns the filename of the selected file. Is empty if no file was selected.
	const std::string &getFileNameP() const override;

	//! Returns the directory of the selected file. Returns NULL, if no directory was selected.
	const std::string &getDirectoryName() const override;

	//! Returns the directory of the selected file converted to wide characters. Returns NULL if no directory was selected.
	const wchar_t *getDirectoryNameW() const override;

	//! called if an event happened.
	bool OnEvent(const core::Event &event) override;

    void updateMesh() override;

	//! draws the element and its children
	void draw() override;

protected:
	void setFileName(const std::string &name);
	void setDirectoryName(const std::string &name);

	//! Ensure filenames are converted correct depending on wide-char settings
    //void pathToStringW(std::wstring &result, const std::string &p);

	//! fills the listbox with files.
	void fillListBox();

	//! sends the event that the file has been selected.
	void sendSelectedEvent(EGUI_EVENT_TYPE type);

	//! sends the event that the file choose process has been canceld
	void sendCancelEvent();

	v2i DragStart;
	std::string FileName;
	std::wstring FileNameW;
	fs::path FileDirectory;
	std::string FileDirectoryFlat;
	std::wstring FileDirectoryFlatW;
	fs::path RestoreDirectory;
	fs::path StartDirectory;

	IGUIButton *CloseButton;
	IGUIButton *OKButton;
	IGUIButton *CancelButton;
	IGUIListBox *FileBox;
	IGUIEditBox *FileNameText;
	IGUIElement *EventParent;
	bool Dragging;
	
    std::unique_ptr<SpriteDrawBatch> drawBatch;
};

} // end namespace gui
