// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIFileOpenDialog.h"

#include "GUISkin.h"
#include "IGUIEnvironment.h"
#include "IGUIButton.h"
#include "IGUIStaticText.h"

namespace gui
{

const s32 FOD_WIDTH = 350;
const s32 FOD_HEIGHT = 250;

//! constructor
CGUIFileOpenDialog::CGUIFileOpenDialog(const wchar_t *title,
		IGUIEnvironment *environment, IGUIElement *parent, s32 id,
        bool restoreCWD, std::string *startDir) :
		IGUIFileOpenDialog(environment, parent, id,
				recti((parent->getAbsolutePosition().getWidth() - FOD_WIDTH) / 2,
						(parent->getAbsolutePosition().getHeight() - FOD_HEIGHT) / 2,
						(parent->getAbsolutePosition().getWidth() - FOD_WIDTH) / 2 + FOD_WIDTH,
						(parent->getAbsolutePosition().getHeight() - FOD_HEIGHT) / 2 + FOD_HEIGHT)),
        FileNameText(0), Dragging(false)
{
	Text = title;

	FileSystem = Environment ? Environment->getFileSystem() : 0;

	if (FileSystem) {
		FileSystem->grab();

		if (restoreCWD)
			RestoreDirectory = FileSystem->getWorkingDirectory();
		if (startDir) {
			StartDirectory = startDir;
			FileSystem->changeWorkingDirectoryTo(startDir);
		}
	} else
		return;

	IGUISpriteBank *sprites = 0;
    img::color8 color(img::white);
	GUISkin *skin = Environment->getSkin();
	if (skin) {
		sprites = skin->getSpriteBank();
		color = skin->getColor(EGDC_WINDOW_SYMBOL);
	}

	const s32 buttonw = skin ? skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) : 2;
	const s32 posx = RelativeRect.getWidth() - buttonw - 4;

	CloseButton = Environment->addButton(recti(posx, 3, posx + buttonw, 3 + buttonw), this, -1,
            L"", skin ? skin->getDefaultText(EGDT_WINDOW_CLOSE).c_str() : L"Close");
	CloseButton->setSubElement(true);
	CloseButton->setTabStop(false);
	if (sprites && skin) {
		CloseButton->setSpriteBank(sprites);
		CloseButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_WINDOW_CLOSE), color);
		CloseButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_WINDOW_CLOSE), color);
	}
    CloseButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
	CloseButton->grab();

	OKButton = Environment->addButton(
			recti(RelativeRect.getWidth() - 80, 30, RelativeRect.getWidth() - 10, 50),
            this, -1, skin ? skin->getDefaultText(EGDT_MSG_BOX_OK).c_str() : L"OK");
	OKButton->setSubElement(true);
    OKButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
	OKButton->grab();

	CancelButton = Environment->addButton(
			recti(RelativeRect.getWidth() - 80, 55, RelativeRect.getWidth() - 10, 75),
            this, -1, skin ? skin->getDefaultText(EGDT_MSG_BOX_CANCEL).c_str() : L"Cancel");
	CancelButton->setSubElement(true);
    CancelButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
	CancelButton->grab();

	FileBox = Environment->addListBox(recti(10, 55, RelativeRect.getWidth() - 90, 230), this, -1, true);
	FileBox->setSubElement(true);
    FileBox->setAlignment(GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT);
	FileBox->grab();

	FileNameText = Environment->addEditBox(0, recti(10, 30, RelativeRect.getWidth() - 90, 50), true, this);
	FileNameText->setSubElement(true);
    FileNameText->setAlignment(GUIAlignment::UpperLeft, EGUIA_LOWERRIGHT, GUIAlignment::UpperLeft, GUIAlignment::UpperLeft);
	FileNameText->grab();

	setTabGroup(true);

	fillListBox();
}

//! destructor
CGUIFileOpenDialog::~CGUIFileOpenDialog()
{
	if (CloseButton)
		CloseButton->drop();

	if (OKButton)
		OKButton->drop();

	if (CancelButton)
		CancelButton->drop();

	if (FileBox)
		FileBox->drop();

	if (FileNameText)
		FileNameText->drop();

	if (FileSystem) {
		// revert to original CWD if path was set in constructor
		if (RestoreDirectory.size())
			FileSystem->changeWorkingDirectoryTo(RestoreDirectory);
		FileSystem->drop();
	}
}

//! returns the filename of the selected file. Returns NULL, if no file was selected.
const wchar_t *CGUIFileOpenDialog::getFileName() const
{
	return FileNameW.c_str();
}

const std::string &CGUIFileOpenDialog::getFileNameP() const
{
	return FileName;
}

//! Returns the directory of the selected file. Returns NULL, if no directory was selected.
const std::string &CGUIFileOpenDialog::getDirectoryName() const
{
	return FileDirectoryFlat;
}

const wchar_t *CGUIFileOpenDialog::getDirectoryNameW() const
{
	return FileDirectoryFlatW.c_str();
}

void CGUIFileOpenDialog::setFileName(const std::string &name)
{
	FileName = name;
	pathToStringW(FileNameW, FileName);
}

void CGUIFileOpenDialog::setDirectoryName(const std::string &name)
{
	FileDirectory = name;
	FileDirectoryFlat = name;
	FileSystem->flattenFilename(FileDirectoryFlat);
	pathToStringW(FileDirectoryFlatW, FileDirectoryFlat);
}

//! called if an event happened.
bool CGUIFileOpenDialog::OnEvent(const main::Event &event)
{
	if (isEnabled()) {
		switch (event.Type) {
		case EET_GUI_EVENT:
			switch (event.GUI.Type) {
			case EGET_ELEMENT_FOCUS_LOST:
				Dragging = false;
				break;
			case EGET_BUTTON_CLICKED:
                if (event.GUI.Caller == CloseButton->getID() ||
                        event.GUI.Caller == CancelButton->getID()) {
					sendCancelEvent();
					remove();
					return true;
                } else if (event.GUI.Caller == OKButton->getID()) {
					if (FileDirectory != L"") {
						sendSelectedEvent(EGET_DIRECTORY_SELECTED);
					}
					if (FileName != L"") {
						sendSelectedEvent(EGET_FILE_SELECTED);
						remove();
						return true;
					}
				}
				break;

			case EGET_LISTBOX_CHANGED: {
				s32 selected = FileBox->getSelected();
				if (FileList && FileSystem) {
					if (FileList->isDirectory(selected)) {
						setFileName("");
						setDirectoryName(FileList->getFullFileName(selected));
					} else {
						setDirectoryName("");
						setFileName(FileList->getFullFileName(selected));
					}
					return true;
				}
			} break;

			case EGET_LISTBOX_SELECTED_AGAIN: {
				const s32 selected = FileBox->getSelected();
				if (FileList && FileSystem) {
					if (FileList->isDirectory(selected)) {
						setDirectoryName(FileList->getFullFileName(selected));
						FileSystem->changeWorkingDirectoryTo(FileDirectory);
						fillListBox();
						setFileName("");
					} else {
						setFileName(FileList->getFullFileName(selected));
					}
					return true;
				}
			} break;
			case EGET_EDITBOX_ENTER:
                if (event.GUI.Caller == FileNameText->getID()) {
					std::string dir(FileNameText->getText());
					if (FileSystem->changeWorkingDirectoryTo(dir)) {
						fillListBox();
						setFileName("");
					}
					return true;
				}
				break;
			default:
				break;
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			switch (event.MouseInput.Type) {
			case EMIE_MOUSE_WHEEL:
				return FileBox->OnEvent(event);
			case EMIE_LMOUSE_PRESSED_DOWN:
				DragStart.X = event.MouseInput.X;
				DragStart.Y = event.MouseInput.Y;
				Dragging = true;
				return true;
			case EMIE_LMOUSE_LEFT_UP:
				Dragging = false;
				return true;
			case EMIE_MOUSE_MOVED:

				if (!event.MouseInput.isLeftPressed())
					Dragging = false;

				if (Dragging) {
					// gui window should not be dragged outside its parent
					if (Parent)
						if (event.MouseInput.X < Parent->getAbsolutePosition().ULC.X + 1 ||
								event.MouseInput.Y < Parent->getAbsolutePosition().ULC.Y + 1 ||
								event.MouseInput.X > Parent->getAbsolutePosition().LRC.X - 1 ||
								event.MouseInput.Y > Parent->getAbsolutePosition().LRC.Y - 1)

							return true;

					move(v2i(event.MouseInput.X - DragStart.X, event.MouseInput.Y - DragStart.Y));
					DragStart.X = event.MouseInput.X;
					DragStart.Y = event.MouseInput.Y;
					return true;
				}
				break;
			default:
				break;
			}
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}

//! draws the element and its children
void CGUIFileOpenDialog::draw()
{
	if (!IsVisible)
		return;

	GUISkin *skin = Environment->getSkin();

	recti rect = AbsoluteRect;

	rect = skin->draw3DWindowBackground(this, true, skin->getColor(EGDC_ACTIVE_BORDER),
			rect, &AbsoluteClippingRect);

	if (Text.size()) {
		rect.ULC.X += 2;
		rect.LRC.X -= skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) + 5;

        render::TTFont *font = skin->getFont(EGDF_WINDOW);
		if (font)
			font->draw(Text.c_str(), rect,
					skin->getColor(EGDC_ACTIVE_CAPTION),
					false, true, &AbsoluteClippingRect);
	}

	IGUIElement::draw();
}

void CGUIFileOpenDialog::pathToStringW(std::wstring &result, const std::string &p)
{
	core::multibyteToWString(result, p);
}

//! fills the listbox with files.
void CGUIFileOpenDialog::fillListBox()
{
	GUISkin *skin = Environment->getSkin();

	if (!FileSystem || !FileBox || !skin)
		return;

	if (FileList)
		FileList->drop();

	FileBox->clear();

	FileList = FileSystem->createFileList();
	std::wstring s;

	if (FileList) {
		for (u32 i = 0; i < FileList->getFileCount(); ++i) {
			pathToStringW(s, FileList->getFileName(i));
			FileBox->addItem(s.c_str(), skin->getIcon(FileList->isDirectory(i) ? EGDI_DIRECTORY : EGDI_FILE));
		}
	}

	if (FileNameText) {
		setDirectoryName(FileSystem->getWorkingDirectory());
		pathToStringW(s, FileDirectory);
		FileNameText->setText(s.c_str());
	}
}

//! sends the event that the file has been selected.
void CGUIFileOpenDialog::sendSelectedEvent(EGUI_EVENT_TYPE type)
{
	main::Event event;
	event.Type = EET_GUI_EVENT;
    event.GUI.Caller = getID();
	event.GUI.Element = 0;
	event.GUI.Type = type;
	Parent->OnEvent(event);
}

//! sends the event that the file choose process has been cancelled
void CGUIFileOpenDialog::sendCancelEvent()
{
	main::Event event;
	event.Type = EET_GUI_EVENT;
    event.GUI.Caller = getID();
	event.GUI.Element = 0;
	event.GUI.Type = EGET_FILE_CHOOSE_DIALOG_CANCELLED;
	Parent->OnEvent(event);
}

} // end namespace gui
