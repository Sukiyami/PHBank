#include "box_viewer.hpp"

#include "ultra_box_viewer.hpp"
#include "savexit_viewer.hpp"
#include "image_manager.hpp"

#include "pokemon.hpp"

#define BOX_HEADER_SELECTED -1
#define SLOT_NO_SELECTION -1

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define BACKGROUND_WIDTH 220
#define BACKGROUND_HEIGHT 210
#define BACKGROUND_SHIFT 40

#define PC_BOX_SHIFT_USED 0
#define BK_BOX_SHIFT_UNUSED PC_BOX_SHIFT_USED + BACKGROUND_WIDTH + BACKGROUND_SHIFT


#define BK_BOX_SHIFT_USED SCREEN_WIDTH - BACKGROUND_WIDTH
#define PC_BOX_SHIFT_UNUSED BK_BOX_SHIFT_USED - BACKGROUND_WIDTH - BACKGROUND_SHIFT


// --------------------------------------------------
/// Compute the current box pointer of the cursor
int16_t* currentBox(CursorBox_t* cursorBox)
// --------------------------------------------------
{
	cursorBox->box = &(cursorBox->inBank ? cursorBox->boxBK : cursorBox->boxPC);
	return cursorBox->box;
}


// --------------------------------------------------
/// Compute the slot and the inslot of the cursor
void computeSlot(CursorBox_t* cursorBox)
// --------------------------------------------------
{
	currentBox(cursorBox);
	cursorBox->inslot = (cursorBox->row == BOX_HEADER_SELECTED ? SLOT_NO_SELECTION : cursorBox->row * BOX_COL_PKMCOUNT + cursorBox->col);
	cursorBox->slot   = (cursorBox->row == BOX_HEADER_SELECTED ? SLOT_NO_SELECTION : *cursorBox->box * BOX_PKMCOUNT + cursorBox->inslot);
}


// --------------------------------------------------
void computeBoxSlot(BoxSlot_t* boxSlot, CursorBox_t* cursorBox)
// --------------------------------------------------
{
	// extractBoxSlot shall already called before

	boxSlot->rowCount = cursorBox->row - boxSlot->row;
	boxSlot->colCount = cursorBox->col - boxSlot->col;

	if (boxSlot->rowCount < 0)
	{
		boxSlot->row = boxSlot->row + boxSlot->rowCount;
		boxSlot->row *= -1;
	}
	if (boxSlot->colCount < 0)
	{
		boxSlot->col = boxSlot->col + boxSlot->colCount;
		boxSlot->col *= -1;
	}

	boxSlot->rowCount++;
	boxSlot->colCount++;
}


// --------------------------------------------------
/// Convert CursorBox position to BoxSlot position
void extractBoxSlot(BoxSlot_t* boxSlot, CursorBox_t* cursorBox)
// --------------------------------------------------
{
	boxSlot->inBank = cursorBox->inBank;
	boxSlot->slot = cursorBox->slot;
	boxSlot->inslot = cursorBox->inslot;
	boxSlot->box = *(cursorBox->box);
	boxSlot->row = cursorBox->row;
	boxSlot->col = cursorBox->col;
}

// --------------------------------------------------
/// Convert BoxSlot position to CursorBox position
void injectBoxSlot(BoxSlot_t* boxSlot, CursorBox_t* cursorBox)
// --------------------------------------------------
{
	cursorBox->inBank = boxSlot->inBank;
	cursorBox->slot = boxSlot->slot;
	cursorBox->inslot = boxSlot->inslot;
	(boxSlot->inBank ? cursorBox->boxBK : cursorBox->boxPC) = boxSlot->box;
	cursorBox->row = boxSlot->row;
	cursorBox->col = boxSlot->col;
}


/*----------------------------------------------------------*\
 |                       Viewer Class                       |
\*----------------------------------------------------------*/


	/*--------------------------------------------------*\
	 |             Constructor / Destructor             |
	\*--------------------------------------------------*/


// --------------------------------------------------
BoxViewer::BoxViewer(Viewer* parent) : Viewer(parent) { }
// --------------------------------------------------


// --------------------------------------------------
BoxViewer::BoxViewer(StateView_e state, Viewer* parent) : Viewer(state, parent) { }
// --------------------------------------------------


// --------------------------------------------------
BoxViewer::~BoxViewer()
// --------------------------------------------------
{
	// Free the textures previoulsy created.
	if (backgroundBox) sf2d_free_texture(backgroundBox);
	if (backgroundResume) sf2d_free_texture(backgroundResume);
	if (icons) sf2d_free_texture(icons);
	if (tiles) sf2d_free_texture(tiles);
}


	/*--------------------------------------------------*\
	 |                  Viewer Methods                  |
	\*--------------------------------------------------*/


		/*------------------------------------------*\
		 |              Public Methods              |
		\*------------------------------------------*/


// --------------------------------------------------
Result BoxViewer::initialize()
// --------------------------------------------------
{
	if (hasChild()) { if (this->child->initialize() == PARENT_STEP) ; else return CHILD_STEP; }

	// Initialize the 2 boxes content
	cursorBox.inBank = true; selectViewBox();
	cursorBox.inBank = false; selectViewBox();

	// Initialise the current Pokémon
	selectViewPokemon();


	// Load textures
	if (!backgroundBox)
		backgroundBox = sf2d_create_texture_mem_RGBA8(ImageManager::boxBackground23o_img.pixel_data, ImageManager::boxBackground23o_img.width, ImageManager::boxBackground23o_img.height, TEXFMT_RGBA8, SF2D_PLACE_RAM);
	if (!backgroundResume)
		backgroundResume = sf2d_create_texture_mem_RGBA8(ImageManager::pokemonResumeBackground_img.pixel_data, ImageManager::pokemonResumeBackground_img.width, ImageManager::pokemonResumeBackground_img.height, TEXFMT_RGBA8, SF2D_PLACE_RAM);
	if (!icons)
		icons = sf2d_create_texture_mem_RGBA8(ImageManager::boxIcons_img.pixel_data, ImageManager::boxIcons_img.width, ImageManager::boxIcons_img.height, TEXFMT_RGBA8, SF2D_PLACE_RAM);
	if (!tiles)
		tiles = sf2d_create_texture_mem_RGBA8(ImageManager::boxTiles_img.pixel_data, ImageManager::boxTiles_img.width, ImageManager::boxTiles_img.height, TEXFMT_RGBA8, SF2D_PLACE_RAM);

	sf2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));

	return PARENT_STEP;
}


// --------------------------------------------------
Result BoxViewer::drawTopScreen()
// --------------------------------------------------
{
	if (hasRegularChild()) { if (this->child->drawTopScreen() == PARENT_STEP); else return CHILD_STEP; }
	
	// Draw the resume background
	sf2d_draw_texture(backgroundResume, 0, 0);

	// If there is a current Pokémon
	if (vPkm.pkm && !vPkm.emptySlot)
	{
		uint32_t x, y;

		x = 32;
		y = 16 - 2;
		// Is the Pokémon an egg?
		if (Pokemon::isEgg(vPkm.pkm))
			sftd_draw_text_white(x, y, "%s", "Egg");
		else
			// Is the Pokémon nicknamed?
			if (Pokemon::isNicknamed(vPkm.pkm))
				sftd_draw_text_white(x, y, "%s", vPkm.NKName);
			else
				sftd_draw_text_white(x, y, "%s", vPkm.species);

		sftd_draw_text_white(x + 168, y, "Lv.%u", vPkm.level);

		x = 11;
		y = 42 - 2;
		sftd_draw_text_white(x, y, "Game's OT");
		sftd_draw_text_white(x+80, y, "%s", PHBank::pKBank()->savedata->OTName);
		sftd_draw_text_white(x, (y += 15), "Dex No.");
		sftd_draw_text_white(x+50, y, "%u", vPkm.speciesID);
		sftd_draw_text_white(x+80, y, "%s", vPkm.species);
		sftd_draw_text_white(x, (y += 15), "O.T.");
		sftd_draw_text_white(x+50, y, "%s", vPkm.OTName);
		sftd_draw_text_white(x, (y += 15), "Stat");
		sftd_draw_text_white(x+90, y, "Value");
		sftd_draw_text_white(x+128, y, "IVs");
		sftd_draw_text_white(x+158, y, "EVs");
		sftd_draw_text_white(x, (y+=15), "HP");
		sftd_draw_text_white(x+100, y, "% 3u", vPkm.stats[Stat::HP]);
		sftd_draw_text_white(x+130, y, "% 2u", vPkm.ivs[Stat::HP]);
		sftd_draw_text_white(x+160, y, "% 3u", vPkm.evs[Stat::HP]);
		sftd_draw_text_white(x, (y+=15), "Attack");
		sftd_draw_text_white(x+100, y, "% 3u", vPkm.stats[Stat::ATK]);
		sftd_draw_text_white(x+130, y, "% 2u", vPkm.ivs[Stat::ATK]);
		sftd_draw_text_white(x+160, y, "% 3u", vPkm.evs[Stat::ATK]);
		sftd_draw_text_white(x, (y+=15), "Defense");
		sftd_draw_text_white(x+100, y, "% 3u", vPkm.stats[Stat::DEF]);
		sftd_draw_text_white(x+130, y, "% 2u", vPkm.ivs[Stat::DEF]);
		sftd_draw_text_white(x+160, y, "% 3u", vPkm.evs[Stat::DEF]);
		sftd_draw_text_white(x, (y+=15), "Sp.Attack");
		sftd_draw_text_white(x+100, y, "% 3u", vPkm.stats[Stat::SPA]);
		sftd_draw_text_white(x+130, y, "% 2u", vPkm.ivs[Stat::SPA]);
		sftd_draw_text_white(x+160, y, "% 3u", vPkm.evs[Stat::SPA]);
		sftd_draw_text_white(x, (y+=15), "Sp.Defense");
		sftd_draw_text_white(x+100, y, "% 3u", vPkm.stats[Stat::SPD]);
		sftd_draw_text_white(x+130, y, "% 2u", vPkm.ivs[Stat::SPD]);
		sftd_draw_text_white(x+160, y, "% 3u", vPkm.evs[Stat::SPD]);
		sftd_draw_text_white(x, (y+=15), "Speed");
		sftd_draw_text_white(x+100, y, "% 3u", vPkm.stats[Stat::SPE]);
		sftd_draw_text_white(x+130, y, "% 2u", vPkm.ivs[Stat::SPE]);
		sftd_draw_text_white(x+160, y, "% 3u", vPkm.evs[Stat::SPE]);
		sftd_draw_text_white(x, (y += 15), "Nature");
		sftd_draw_text_white(x+50, y, "%s", vPkm.nature);
		sftd_draw_text_white(x, (y += 15), "Ability");
		sftd_draw_text_white(x+50, y, "%s", vPkm.ability);
		sftd_draw_text_white(x, (y += 15), "Item");
		sftd_draw_text_white(x+50, y, "%s", vPkm.item);

		x = 246;
		y = 147 - 15 - 2;
		sftd_draw_text_white(x, (y += 15), "Hidden Power: %s", vPkm.hiddenPower);
		sftd_draw_text_white(x, (y += 15), "Moves");
		sftd_draw_text_white(x, (y += 15), " %s", vPkm.moves[0]);
		sftd_draw_text_white(x, (y += 15), " %s", vPkm.moves[1]);
		sftd_draw_text_white(x, (y += 15), " %s", vPkm.moves[2]);
		sftd_draw_text_white(x, (y += 15), " %s", vPkm.moves[3]);


		sf2d_draw_texture_part_scale(icons, 256, 48, ((vPkm.speciesID-1) % 25) * 40, ((vPkm.speciesID-1) / 25) * 30, 40, 30, 3.0f, 3.0f);


		if (vPkm.isShiny)
		{
			sf2d_draw_texture_part(tiles, 240, 135, 57, 73, 9, 9);
		}

		if (vPkm.isKalosBorn)
		{
			sf2d_draw_texture_part(tiles, 250, 135, 66, 73, 9, 9);
		}

		if (vPkm.isCured)
		{
			sf2d_draw_texture_part(tiles, 260, 135, 75, 73, 9, 9);
		}

	}

	if (hasOverlayChild()) { this->child->drawTopScreen(); }
	return SUCCESS_STEP;
}


// --------------------------------------------------
Result BoxViewer::drawBotScreen()
// --------------------------------------------------
{
	if (hasRegularChild()) { if (this->child->drawBotScreen() == PARENT_STEP); else return CHILD_STEP; }
	
	int16_t boxShift;
	box_t** vBox = NULL;


	// TODO: Merge the Pokémon box draw function for both boxes (DRY)
	//       Draw selected Pokémon AFTER the both boxes background/buttons


	// Draw the current box (PC|BK) data
	{
		// Retrieve the current box, and the drawing offset.
		boxShift = (cursorBox.inBank ? BK_BOX_SHIFT_USED : PC_BOX_SHIFT_USED);
		vBox = &(cursorBox.inBank ? vBKBox : vPCBox);

		// Draw the box background and the box title
		sf2d_draw_texture(backgroundBox, boxShift, 20);
		char boxTitle[0x18];
		snprintf(boxTitle, 0x18, "Box %i", (cursorBox.inBank ? cursorBox.boxBK : cursorBox.boxPC) + 1);
		int boxTitleWidth = sftd_get_text_width(PHBank::font(), 13, boxTitle);
		sftd_draw_text(PHBank::font(), boxShift + (BACKGROUND_WIDTH - boxTitleWidth) / 2, 25, RGBA8(0x00, 0x00, 0x00, 0xFF), 13, boxTitle);

		
		// If there is a Pokémon currently selected
		if (isPkmDragged || isPkmHeld)
			// if (sPkm.pkm)
		{
			// Draw Pokémon icons
			for (uint32_t i = 0; i < 30; i++)
			{
				// If the Pokémon isn't the selected Pokémon
				if (sPkm != &((*vBox)->slot[i]))
				{
					// If the Pokémon is an egg
					if (Pokemon::isEgg(&((*vBox)->slot[i])))
					{
						// Draw the egg icon
						sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, ((PKM_COUNT) % 25) * 40, ((PKM_COUNT) / 25) * 30, 40, 30);
					}
					else
					{
						// Draw the Pokémon icon
						sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, (((*vBox)->slot[i].speciesID-1) % 25) * 40, (((*vBox)->slot[i].speciesID-1) / 25) * 30, 40, 30);
					}
				}
			}
		}
		// If there is no Pokémon currently selected
		else
		{
			// Draw Pokémon icons
			for (uint32_t i = 0; i < 30; i++)
			{
				// If the Pokémon is an egg
				if (Pokemon::isEgg(&((*vBox)->slot[i])))
				{
					// Draw the egg icon
					sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, ((PKM_COUNT) % 25) * 40, ((PKM_COUNT) / 25) * 30, 40, 30);
				}
				else
				{
					// Draw the Pokémon icon
					sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, (((*vBox)->slot[i].speciesID-1) % 25) * 40, (((*vBox)->slot[i].speciesID-1) / 25) * 30, 40, 30);
				}
			}
		}
		

		// Draw CursorType buttons (Red|Blue|Green)
		sf2d_draw_texture_part(tiles, boxShift + 21 +   0, 0,   0, 0, 64, 32);
		sf2d_draw_texture_part(tiles, boxShift + 21 +  64, 0,  64, 0, 64, 32);
		sf2d_draw_texture_part(tiles, boxShift + 21 + 128, 0, 128, 0, 64, 32);

		// Draw the SwapBox buttons
		sf2d_draw_texture_part(tiles, boxShift + 10 +  0, 20,   0, 64, 16, 24);
		sf2d_draw_texture_part(tiles, boxShift + BACKGROUND_WIDTH - 24, 20,  16, 64, 16, 24);


		// If a Pokémon is currently selected
		if (sPkm)
		{
			// If the selected Pokémon is dragged
			if (isPkmDragged)
			{
				// Draw dragged Pokémon icon under the stylus
				sf2d_draw_texture_part(icons, touch.px - 16, touch.py - 16, ((sPkm->speciesID-1) % 25) * 40, ((sPkm->speciesID-1) / 25) * 30, 40, 30);
			}
			// If the selected Pokémon is held
			else // (isPkmHeld)
			{
				if (cursorBox.inslot == SLOT_NO_SELECTION)
				{
					// Draw the Pokémon icon on the box title
					sf2d_draw_texture_part(icons, boxShift + 105, 8, ((sPkm->speciesID-1) % 25) * 40, ((sPkm->speciesID-1) / 25) * 30, 40, 30);
				}
				else
				{
					// Draw the Pokémon icon on the current slot a bit shifted
					sf2d_draw_texture_part(icons, boxShift + 12 + (cursorBox.inslot % 6) * 35, 20 + 13 + (cursorBox.inslot / 6) * 35, ((sPkm->speciesID-1) % 25) * 40, ((sPkm->speciesID-1) / 25) * 30, 40, 30);
				}
			}
		}
		else
		{
			// Draw the cursor
			if (cursorBox.inslot == SLOT_NO_SELECTION)
			{
				// Draw the cursor icon on the box title
				sf2d_draw_texture_part(tiles, boxShift + 105, 8 - cursorPositionOffY, 32 * cursorType, 32, 32, 32);
			}
			else
			{
				// Draw the cursor icon on the current slot a bit shifted
				sf2d_draw_texture_part(tiles, boxShift + 17 + (cursorBox.inslot % 6) * 35, 20 + 13 + (cursorBox.inslot / 6) * 35 - cursorPositionOffY, 32 * cursorType, 32, 32, 32);
			}
		}
	}


	// Draw the other box (PC|BK) data
	{
		// Retrieve the current box, and the drawing offset
		boxShift = (cursorBox.inBank ? PC_BOX_SHIFT_UNUSED : BK_BOX_SHIFT_UNUSED);
		vBox = &(cursorBox.inBank ? vPCBox : vBKBox);

		// Draw the box background
		sf2d_draw_texture(backgroundBox, boxShift, 20);
		

		// Draw the SwapBox buttons
		sf2d_draw_texture_part(tiles, boxShift + 10 +  0, 20,   0, 64, 16, 24);
		sf2d_draw_texture_part(tiles, boxShift + BACKGROUND_WIDTH - 24, 20,  16, 64, 16, 24);


		// If there is a Pokémon currently selected
		if (isPkmDragged || isPkmHeld)
			// if (sPkm.pkm)
		{
			// Draw Pokémon icons
			for (uint32_t i = 0; i < 30; i++)
			{
				// If the Pokémon isn't the selected Pokémon
				if (sPkm != &((*vBox)->slot[i]))
				{
					// If the Pokémon is an egg
					if (Pokemon::isEgg(&((*vBox)->slot[i])))
					{
						// Draw the egg icon
						sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, ((PKM_COUNT) % 25) * 40, ((PKM_COUNT) / 25) * 30, 40, 30);
					}
					else
					{
						// Draw the Pokémon icon
						sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, (((*vBox)->slot[i].speciesID-1) % 25) * 40, (((*vBox)->slot[i].speciesID-1) / 25) * 30, 40, 30);
					}
				}
			}
		}
		// If there is no Pokémon currently selected
		else
		{
			// Draw Pokémon icons
			for (uint32_t i = 0; i < 30; i++)
			{
				// If the Pokémon is an egg
				if (Pokemon::isEgg(&((*vBox)->slot[i])))
				{
					// Draw the egg icon
					sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, ((PKM_COUNT) % 25) * 40, ((PKM_COUNT) / 25) * 30, 40, 30);
				}
				else
				{
					// Draw the Pokémon icon
					sf2d_draw_texture_part(icons, boxShift + (i % BOX_COL_PKMCOUNT) * 35, (i / BOX_COL_PKMCOUNT) * 35 + 50, (((*vBox)->slot[i].speciesID-1) % 25) * 40, (((*vBox)->slot[i].speciesID-1) / 25) * 30, 40, 30);
				}
			}
		}
	}


	if (hasOverlayChild()) { this->child->drawBotScreen(); }
	return SUCCESS_STEP;
}


// --------------------------------------------------
Result BoxViewer::updateControls(const u32& kDown, const u32& kHeld, const u32& kUp, const touchPosition* touch)
// --------------------------------------------------
{
	if (hasRegularChild() || hasOverlayChild()) { if (this->child->updateControls(kDown, kHeld, kUp, touch) == PARENT_STEP); else return CHILD_STEP; }
	
	if (kDown & KEY_START)
	{
		// Open the Savexit popup
		new SavexitViewer(StateView::Overlay, this);
		child->initialize();
		return CHILD_STEP;
	}


	{
		bool boolMod = false;
		int16_t boxMod = 0;
		int16_t rowMod = 0;
		int16_t colMod = 0;


		if (kDown & KEY_L) boxMod--;
		else if (kDown & KEY_R) boxMod++;

		if (kDown & KEY_UP) rowMod--;
		else if (kDown & KEY_DOWN) rowMod++;

		if (kDown & KEY_LEFT) { if (cursorBox.row == BOX_HEADER_SELECTED) boxMod--; else colMod--; }
		else if (kDown & KEY_RIGHT) { if (cursorBox.row == BOX_HEADER_SELECTED) boxMod++; else colMod++; }

		if (kDown & KEY_ZL) { cursorBox.inBank = false; boolMod = true; }
		else if (kDown & KEY_ZR) { cursorBox.inBank = true; boolMod = true; }

		if (kDown & KEY_TOUCH)
		{
			int16_t boxShift = (cursorBox.inBank ? BK_BOX_SHIFT_USED : PC_BOX_SHIFT_USED);

			// If the SwapBox buttons are touched down
			if (touchWithin(touch->px, touch->py, boxShift + 10, 20, 16, 24)) boxMod--;
			else if (touchWithin(touch->px, touch->py, boxShift + 196, 20, 16, 24)) boxMod++;

			boxShift = (cursorBox.inBank ? PC_BOX_SHIFT_UNUSED : BK_BOX_SHIFT_UNUSED);

			// If the other box (PC|BK) is touched down
			if (touchWithin(touch->px, touch->py, boxShift, 20, BACKGROUND_WIDTH, BACKGROUND_HEIGHT)) { cursorBox.inBank = !cursorBox.inBank; boolMod = true; }
		}
		else if (kHeld & KEY_TOUCH)
		{
			// If there is a Pokémon currently dragged
			if (isPkmDragged)
			{
				int16_t boxShift = (cursorBox.inBank ? BK_BOX_SHIFT_USED : PC_BOX_SHIFT_USED);
				// If the SwapBox buttons are touched held once
				if (touchWithin(touch->px, touch->py, boxShift + 10, 20, 16, 24) && !touchWithin(this->touch.px, this->touch.py, boxShift + 10, 20, 16, 24)) boxMod--;
				else if (touchWithin(touch->px, touch->py, boxShift + 196, 20, 16, 24) && !touchWithin(this->touch.px, this->touch.py, boxShift + 196, 20, 16, 24)) boxMod++;

				boxShift = (cursorBox.inBank ? PC_BOX_SHIFT_UNUSED : BK_BOX_SHIFT_UNUSED);

				// If the other box (PC|BK) is touched held
				if (touchWithin(touch->px, touch->py, boxShift, 20, BACKGROUND_WIDTH, BACKGROUND_HEIGHT)) { cursorBox.inBank = !cursorBox.inBank; boolMod = true; }
			}
		}

		if (boxMod || rowMod || colMod)
		{
			currentBox(&cursorBox);
			*cursorBox.box += boxMod;
			cursorBox.row += rowMod;
			cursorBox.col += colMod;
			
			if (*cursorBox.box < 0) *cursorBox.box = (cursorBox.inBank ? BANK_BOXCOUNT : PC_BOXCOUNT)-1;
			else if (*cursorBox.box > (cursorBox.inBank ? BANK_BOXCOUNT : PC_BOXCOUNT)-1) *cursorBox.box = 0;

			if (cursorBox.row < BOX_HEADER_SELECTED) cursorBox.row = BOX_ROW_PKMCOUNT-1;
			else if (cursorBox.row > BOX_ROW_PKMCOUNT-1) cursorBox.row = BOX_HEADER_SELECTED;

			if (cursorBox.col < 0) { cursorBox.col = BOX_COL_PKMCOUNT-1; cursorBox.inBank = !cursorBox.inBank; }
			else if (cursorBox.col > BOX_COL_PKMCOUNT-1) { cursorBox.col = 0; cursorBox.inBank = !cursorBox.inBank; }

			boolMod = true;
		}

		if (boolMod)
		{
			// Update the current Box/Pokémon displayed
			currentBox(&cursorBox);
			selectViewBox();
			selectViewPokemon();
		}
	}

	if (kDown & KEY_SELECT)
	{
		switchCursorType();
	}

	if (cursorType == CursorType::SingleSelect)
	{
		if (kDown & KEY_A && !isPkmDragged)
		{
			// Move a Pokémon (grab and drop)
			selectMovePokemon();
		}

		if (kDown & KEY_B && !isPkmDragged)
		{
			// Drop a Pokémon
			cancelMovePokemon();
		}
	}
	else if (cursorType == CursorType::QuickSelect)
	{
		if (kDown & KEY_A && !isPkmDragged)
		{
			// Move a Pokémon (grab and drop)
			selectMovePokemon();
		}

		if (kDown & KEY_B && !isPkmDragged)
		{
			// Drop a Pokémon
			cancelMovePokemon();
		}

		if (kDown & KEY_Y && !isPkmDragged)
		{
			if (vPCBox && vBKBox) // TODO: Is the `if` useless?
			{
				// Swap the two boxes (PC|BK)
				PHBank::pKBank()->moveBox(cursorBox.boxPC, false, cursorBox.boxBK, true);
			}
		}
	}
	else if (cursorType == CursorType::MultipleSelect)
	{
		if (kDown & KEY_A)
		{
			// Move a Pokémon (grab and drop)
			selectMovePokemon();

			// TODO: Feature to Move Pokémon (multiple-grab and multiple-drop)
			// selectMultiMovePokemon();

		}

		if (kDown & KEY_Y)
		{
			// Open the UltraBox viewer
			UltraBoxViewer* ultraBoxViewer = new UltraBoxViewer(StateView::Regular, this);
			ultraBoxViewer->selectViewBox(*cursorBox.box, cursorBox.inBank);
			child->initialize();
			return CHILD_STEP;
		}
	}
	

	{
		if (kDown & KEY_TOUCH)
		{
			int16_t boxShift = (cursorBox.inBank ? BK_BOX_SHIFT_USED : PC_BOX_SHIFT_USED);
			uint16_t px = touch->px;
			uint16_t py = touch->py;

			// If the TouchArea is within the SingleSelect CursorType button area
			if (touchWithin(px, py, boxShift + 22, 0, 59, 16))
			{
				selectCursorType(CursorType::SingleSelect);
			}
			// If the TouchArea is within the QuickSelect CursorType button area
			else if (touchWithin(px, py, boxShift + 86, 0, 59, 16))
			{
				selectCursorType(CursorType::QuickSelect);
			}
			// If the TouchArea is within the MultipleSelect CursorType button area
			else if (touchWithin(px, py, boxShift + 150, 0, 59, 16))
			{
				selectCursorType(CursorType::MultipleSelect);
			}
			// If the TouchArea is within the Pokémon icons area of the box
			else if (touchWithin(px, py, boxShift, 50, BACKGROUND_WIDTH, BACKGROUND_HEIGHT - 30))
			{
				// If no Pokémon is currently selected or dragged
				if (!sPkm && !isPkmDragged)
				{
					// Move the cursor to the new slot
					cursorBox.row = ((py - 50) / 35);
					cursorBox.col = ((px - boxShift) / 35);
					
					// Update the current Pokémon
					selectViewPokemon();
					// Move the current Pokémon (grab)
					selectMovePokemon();

					if (!vPkm.emptySlot)
						isPkmDragged = true;
				}
				// If no Pokémon is currently selected and dragged
				else
				{
					uint16_t oldRow = cursorBox.row;
					uint16_t oldCol = cursorBox.col;

					// Move the cursor to the new slot
					cursorBox.row = ((py - 50) / 35);
					cursorBox.col = ((px - boxShift) / 35);

					// Update the current Pokémon
					selectViewPokemon();
					// And drop the Pokémon if one is held and it is the same current slot (mean double tap to move a held Pokémon)
					if (isPkmHeld && oldRow == cursorBox.row && oldCol == cursorBox.col)
						selectMovePokemon();
				}
			}
		}
		else if (kHeld & KEY_TOUCH)
		{
			// Keep the last touched held position
			this->touch.px = touch->px;
			this->touch.py = touch->py;
		}
		else if (kUp & KEY_TOUCH)
		{
			touch = &(this->touch);

			int16_t boxShift = (cursorBox.inBank ? BK_BOX_SHIFT_USED : PC_BOX_SHIFT_USED);
			uint16_t px = touch->px;
			uint16_t py = touch->py;

			// If a Pokémon is currently dragged
			if (sPkm && isPkmDragged)
			{
				// If the TouchArea is within the Pokémon icons area of the box
				if (touchWithin(px, py, boxShift, 50, BACKGROUND_WIDTH, BACKGROUND_HEIGHT))
				{
					// Move the cursor to the new slot
					cursorBox.row = ((py - 50) / 35);
					cursorBox.col = ((px - boxShift) / 35);

					// Update the current Pokémon
					selectViewPokemon();
					// Move the dragged Pokémon (drop)
					selectMovePokemon();
				}
				else
				{
					// Cancel the Pokémon dragging
					cancelMovePokemon();

					// Return to the old dragged Pokémon slot?
					// injectBoxSlot(&sSlot, &cursorBox);
					// selectViewBox();

					// Update the current Pokémon
					selectViewPokemon();
				}

				isPkmDragged = false;
			}
		}
	}

	cursorPositionOffY += cursorPositionShiftY;
	if (cursorPositionOffY < 0 || cursorPositionOffY > cursorPositionMaxY)
		cursorPositionShiftY *= -1;

	return SUCCESS_STEP;
}


// --------------------------------------------------
void BoxViewer::selectViewBox(uint16_t boxID, bool inBank)
// --------------------------------------------------
{
	// Change a box ID and update both displayed boxes (PC|BK)

	bool BK = cursorBox.inBank;
	cursorBox.inBank = inBank;

	if (inBank)
		cursorBox.boxBK = boxID;
	else
		cursorBox.boxPC = boxID;

	selectViewBox();

	cursorBox.inBank = BK;
	selectViewBox();
}


		/*------------------------------------------*\
		 |             Protected Methods            |
		\*------------------------------------------*/


		/*------------------------------------------*\
		 |              Private Methods             |
		\*------------------------------------------*/



// --------------------------------------------------
void BoxViewer::selectCursorType(CursorType_e cursorType)
// --------------------------------------------------
{
	this->cursorType = cursorType;
}


// --------------------------------------------------
void BoxViewer::switchCursorType()
// --------------------------------------------------
{
	// ... -> Single -> Quick -> Multiple -> ... //
	if (cursorType == CursorType::SingleSelect)
		selectCursorType(CursorType::QuickSelect);
	else if (cursorType == CursorType::QuickSelect)
		selectCursorType(CursorType::MultipleSelect);
	else if (cursorType == CursorType::MultipleSelect)
		selectCursorType(CursorType::SingleSelect);
}


// --------------------------------------------------
void BoxViewer::selectViewBox()
// --------------------------------------------------
{
	// Compute the current cursor slot
	computeSlot(&cursorBox);

	box_t** vBox = NULL;
	if (cursorBox.inBank)
		vBox = &vBKBox;
	else
		vBox = &vPCBox;

	PHBank::pKBank()->getBox(*cursorBox.box, vBox, cursorBox.inBank);
}


// --------------------------------------------------
void BoxViewer::selectViewPokemon()
// --------------------------------------------------
{
	// Compute the current cursor slot
	computeSlot(&cursorBox);

	// If the cursor isn't in a box slot
	if (cursorBox.slot == SLOT_NO_SELECTION)
	{
		vPkm.pkm = NULL;
	}
	// If the cursor is in a box slot
	else
	{
		PHBank::pKBank()->getPkm(*cursorBox.box, cursorBox.inslot, &vPkm.pkm, cursorBox.inBank);
		populateVPkmData(&vPkm);
	}
}


// --------------------------------------------------
void BoxViewer::selectMovePokemon()
// --------------------------------------------------
{
	computeSlot(&cursorBox);
	// selectViewPokemon();

	// If no Pokémon is currently selected
	if (!sPkm)
	{
		// If the current Pokémon slot isn't empty (to avoid empty slot move)
		if (!vPkm.emptySlot)
		{
			// Select the current Pokémon
			sPkm = vPkm.pkm;
			extractBoxSlot(&sSlot, &cursorBox);
			if (!isPkmDragged) isPkmHeld = true;
		}
	}
	// Else if there is a current Pokémon
	else if (vPkm.pkm)
	{
		// If the selected Pokémon isn't the current Pokémon
		if (sPkm != vPkm.pkm)
		{
			// Swap the Pokémon currenlty selected and the current Pokémon, and keep the return value (true: had moved, false: hadn't)
			bool moved = PHBank::pKBank()->movePkm(sPkm, vPkm.pkm, sSlot.inBank, cursorBox.inBank);

			// If the Pokémon had moved
			if (moved)
			{
				// Cancel the selection
				cancelMovePokemon();
			}
			// And populate the Pokémon data
			populateVPkmData(&vPkm);
		}
		else
		{
			// Cancel the selection, since it's moved to the same slot
			cancelMovePokemon();
		}
	}
}


// --------------------------------------------------
void BoxViewer::selectMultiMovePokemon()
// --------------------------------------------------
{
	computeSlot(&cursorBox);

	// If no Pokémon is currently selected
	if (!sPkm)
	{
		sPkm = vPkm.pkm;
		extractBoxSlot(&sSlot, &cursorBox);
		isPkmMSelecting = true;
	}
	else if (isPkmMSelecting)
	{
		computeBoxSlot(&sSlot, &cursorBox);
		injectBoxSlot(&sSlot, &cursorBox);
		isPkmMSelecting = false;
		isPkmMDragged = true;
	}
	else if (isPkmMDragged)
	{

	}
}


// --------------------------------------------------
void BoxViewer::cancelMovePokemon()
// --------------------------------------------------
{
	// Reset the selection

	sPkm = NULL;
	isPkmHeld = false;
	isPkmDragged = false;
	isPkmMDragged = false;
	isPkmMSelecting = false;
}


// --------------------------------------------------
void BoxViewer::populateVPkmData(vPkm_t* vPkm)
// --------------------------------------------------
{
	u16* name;

	name = Pokemon::NK_name(vPkm->pkm);
	if (name)
	{
		for (u8 i = 0; i < 0x18; i += 2)
			vPkm->NKName[i / 2] = name[i / 2] & 0xFF;
		utf16_to_utf8(vPkm->NKName, name, 0x18 / 2);
		vPkm->NKName[0x1a / 2 - 1] = '\0';
	}
	else
	{
		vPkm->NKName[0] = '\0';
	}

	name = Pokemon::OT_name(vPkm->pkm);
	if (name)
	{
		for (u8 i = 0; i < 0x18; i += 2)
			vPkm->OTName[i / 2] = name[i / 2] & 0xFF;
		utf16_to_utf8(vPkm->OTName, name, 0x18 / 2);
		vPkm->OTName[0x1a / 2 - 1] = '\0';
	}
	else
	{
		vPkm->OTName[0] = '\0';
	}

	name = Pokemon::HT_name(vPkm->pkm);
	if (name)
	{
		for (u8 i = 0; i < 0x18; i += 2)
			vPkm->HTName[i / 2] = name[i / 2] & 0xFF;
		utf16_to_utf8(vPkm->HTName, name, 0x18 / 2);
		vPkm->HTName[0x1a / 2 - 1] = '\0';
	}
	else
	{
		vPkm->HTName[0] = '\0';
	}
	

	vPkm->emptySlot = PHBank::pKBank()->isPkmEmpty(vPkm->pkm);

	if (Pokemon::isEgg(vPkm->pkm))
	{
		vPkm->isShiny = Pokemon::isShiny(vPkm->pkm, PHBank::pKBank()->savedata->TID, PHBank::pKBank()->savedata->SID);
	}
	else
	{
		vPkm->isShiny = Pokemon::isShiny(vPkm->pkm);
	}

	vPkm->isKalosBorn = Pokemon::isKalosBorn(vPkm->pkm);
	vPkm->isInfected = Pokemon::isInfected(vPkm->pkm);
	vPkm->isCured = Pokemon::isCured(vPkm->pkm);


	vPkm->speciesID = Pokemon::speciesID(vPkm->pkm);
	vPkm->species = PKData::species(vPkm->speciesID);
	vPkm->item = PKData::items(Pokemon::itemID(vPkm->pkm));
	vPkm->nature = PKData::natures(Pokemon::nature(vPkm->pkm));
	vPkm->ability = PKData::abilities(Pokemon::ability(vPkm->pkm));
	vPkm->hiddenPower = PKData::HPTypes(Pokemon::HPType(vPkm->pkm));

	vPkm->moves[0] = PKData::moves(Pokemon::move1(vPkm->pkm));
	vPkm->moves[1] = PKData::moves(Pokemon::move2(vPkm->pkm));
	vPkm->moves[2] = PKData::moves(Pokemon::move3(vPkm->pkm));
	vPkm->moves[3] = PKData::moves(Pokemon::move4(vPkm->pkm));

	vPkm->level = Pokemon::level(vPkm->pkm);
	vPkm->stats[Stat::HP] = Pokemon::HP(vPkm->pkm);
	vPkm->stats[Stat::ATK] = Pokemon::ATK(vPkm->pkm);
	vPkm->stats[Stat::DEF] = Pokemon::DEF(vPkm->pkm);
	vPkm->stats[Stat::SPA] = Pokemon::SPA(vPkm->pkm);
	vPkm->stats[Stat::SPD] = Pokemon::SPD(vPkm->pkm);
	vPkm->stats[Stat::SPE] = Pokemon::SPE(vPkm->pkm);
	vPkm->ivs[Stat::HP] = Pokemon::IV_HP(vPkm->pkm);
	vPkm->ivs[Stat::ATK] = Pokemon::IV_ATK(vPkm->pkm);
	vPkm->ivs[Stat::DEF] = Pokemon::IV_DEF(vPkm->pkm);
	vPkm->ivs[Stat::SPA] = Pokemon::IV_SPA(vPkm->pkm);
	vPkm->ivs[Stat::SPD] = Pokemon::IV_SPD(vPkm->pkm);
	vPkm->ivs[Stat::SPE] = Pokemon::IV_SPE(vPkm->pkm);
	vPkm->evs[Stat::HP] = Pokemon::EV_HP(vPkm->pkm);
	vPkm->evs[Stat::ATK] = Pokemon::EV_ATK(vPkm->pkm);
	vPkm->evs[Stat::DEF] = Pokemon::EV_DEF(vPkm->pkm);
	vPkm->evs[Stat::SPA] = Pokemon::EV_SPA(vPkm->pkm);
	vPkm->evs[Stat::SPD] = Pokemon::EV_SPD(vPkm->pkm);
	vPkm->evs[Stat::SPE] = Pokemon::EV_SPE(vPkm->pkm);
}