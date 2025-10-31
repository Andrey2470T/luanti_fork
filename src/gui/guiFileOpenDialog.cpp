// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiFileOpenDialog.h"

#include "GUISkin.h"
#include "IGUIStaticText.h"
#include "client/ui/sprite.h"
#include "client/render/rendersystem.h"
#include "client/ui/text_sprite.h"
#include "util/enriched_string.h"
#include "util/string.h"

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
        FileNameText(0), Dragging(false),
        DialogBank(std::make_unique<UISpriteBank>(environment->getRenderSystem(),
        	environment->getResourceCache(), false))
{
	Text = title;

	if (restoreCWD)
		RestoreDirectory = fs::current_path();
	if (startDir) {
        StartDirectory = *startDir;
        fs::current_path(*startDir);
	}

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
	
	DialogBank->addSprite({}, 0);
    DialogBank->addTextSprite(environment->getRenderSystem()->getFontManager(), {}, 0);
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

	// revert to original CWD if path was set in constructor
	if (!RestoreDirectory.empty())
		fs::current_path(RestoreDirectory);
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
    //pathToStringW(FileNameW, FileName);
}

void CGUIFileOpenDialog::setDirectoryName(const std::string &name)
{
	FileDirectory = name;
	FileDirectoryFlat = name;
	//FileSystem->flattenFilename(FileDirectoryFlat);
    //pathToStringW(FileDirectoryFlatW, FileDirectoryFlat);
}

//! called if an event happened.
bool CGUIFileOpenDialog::OnEvent(const core::Event &event)
{
	if (isEnabled()) {
		switch (event.Type) {
		case EET_GUI_EVENT:
			switch (event.GUI.Type) {
			case EGET_ELEMENT_FOCUS_LOST:
				Dragging = false;
				break;
			case EGET_BUTTON_CLICKED:
                if (event.GUI.Caller == CloseButton ||
                        event.GUI.Caller == CancelButton) {
					sendCancelEvent();
					remove();
					return true;
                } else if (event.GUI.Caller == OKButton) {
                    if (!FileDirectory.empty()) {
						sendSelectedEvent(EGET_DIRECTORY_SELECTED);
					}
                    if (!FileName.empty()) {
						sendSelectedEvent(EGET_FILE_SELECTED);
						remove();
						return true;
					}
				}
				break;

			case EGET_LISTBOX_CHANGED: {
                std::string selected = wide_to_utf8(FileBox->getListItem(FileBox->getSelected()));
                fs::path p = fs::absolute(selected);
                if (fs::is_directory(p)) {
                    setFileName("");
                    setDirectoryName(p);
                } else {
                    setDirectoryName("");
                    setFileName(p);
                }
                return true;
			} break;

			case EGET_LISTBOX_SELECTED_AGAIN: {
                std::string selected = wide_to_utf8(FileBox->getListItem(FileBox->getSelected()));
                fs::path p = fs::absolute(selected);
                if (fs::is_directory(p)) {
                    setDirectoryName(p);
                    fs::current_path(FileDirectory);
                    fillListBox();
                    setFileName("");
                } else {
                    setFileName(p);
                }
                return true;
			} break;
			case EGET_EDITBOX_ENTER:
                if (event.GUI.Caller == FileNameText) {
					fs::path dir(FileNameText->getText());
                    if (fs::exists(dir)) {
						fs::current_path(dir);
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

	auto sprite0 = DialogBank->getSprite(0);
	sprite0->clear();

    rectf cliprect = toRectf(AbsoluteClippingRect);
    skin->add3DWindowBackground(sprite0, true, skin->getColor(EGDC_ACTIVE_BORDER), toRectf(rect), &cliprect);
    sprite0->rebuildMesh();
    AbsoluteClippingRect = toRecti(cliprect);
    sprite0->setClipRect(AbsoluteClippingRect);

	if (Text.size()) {
		rect.ULC.X += 2;
		rect.LRC.X -= skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) + 5;

        render::TTFont *font = skin->getFont(EGDF_WINDOW);
		if (font) {
            auto sprite1 = dynamic_cast<UITextSprite *>(DialogBank->getSprite(1));
            sprite1->setText(Text);
            sprite1->setOverrideColor(skin->getColor(EGDC_ACTIVE_CAPTION));
            sprite1->updateBuffer(toRectf(rect));
            sprite1->setClipRect(AbsoluteClippingRect);
		}
	}

    DialogBank->drawBank();

	IGUIElement::draw();
}

//! fills the listbox with files.
void CGUIFileOpenDialog::fillListBox()
{
	GUISkin *skin = Environment->getSkin();

	if (!FileBox || !skin)
		return;

	FileBox->clear();

	std::wstring s;

    fs::path cur_dir = fs::current_path();
    for (const auto &dir_entry : fs::recursive_directory_iterator(cur_dir)) {
        std::wstring entry_name = utf8_to_wide(dir_entry.path().stem().string());
        FileBox->addItem(entry_name.c_str(), skin->getIcon(fs::is_directory(dir_entry.path()) ? EGDI_DIRECTORY : EGDI_FILE));
	}

	if (FileNameText) {
		setDirectoryName(fs::current_path().string());

        std::wstring filedir = utf8_to_wide(FileDirectory.string());
        FileNameText->setText(filedir.c_str());
	}
}

//! sends the event that the file has been selected.
void CGUIFileOpenDialog::sendSelectedEvent(EGUI_EVENT_TYPE type)
{
	core::Event event;
	event.Type = EET_GUI_EVENT;
    event.GUI.Caller = this;
    event.GUI.Element = 0;
	event.GUI.Type = type;
	Parent->OnEvent(event);
}

//! sends the event that the file choose process has been cancelled
void CGUIFileOpenDialog::sendCancelEvent()
{
	core::Event event;
	event.Type = EET_GUI_EVENT;
    event.GUI.Caller = this;
    event.GUI.Element = 0;
	event.GUI.Type = EGET_FILE_CHOOSE_DIALOG_CANCELLED;
	Parent->OnEvent(event);
}

} // end namespace gui
