#pragma once
#include "OptionView.hh"
#include <util/cLang.h>

static void setupMDInput();

class MdOptionView : public OptionView
{
private:

	struct SixButtonPadMenuItem : public BoolMenuItem
	{
		void init(bool on) { BoolMenuItem::init("6-button Gamepad", on); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			option6BtnPad = on;
			setupMDInput();
			vController.place();
		}
	} sixButtonPad;

	struct MultitapMenuItem : public BoolMenuItem
	{
		void init(bool on) { BoolMenuItem::init("4-Player Adapter", on); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			usingMultiTap = on;
			setupMDInput();
		}
	} multitap;

	struct smsFMMenuItem : public BoolMenuItem
	{
		void init() { BoolMenuItem::init("MarkIII FM Sound Unit", optionSmsFM); }

		void select(View *view, const InputEvent &e)
		{
			toggle();
			optionSmsFM = on;
			config_ym2413_enabled = optionSmsFM;
		}
	} smsFM;

	struct BigEndianSramMenuItem : public BoolMenuItem, public YesNoAlertView::Callback
	{
		void init() { BoolMenuItem::init("Use Big-Endian SRAM", optionBigEndianSram); }

		void onSelect()
		{
			toggle();
			optionBigEndianSram = on;
		}

		void select(View *view, const InputEvent &e)
		{
			ynAlertView.init("Warning, this changes the format of SRAM saves files. "
					"Turn on to make them compatible with other emulators like Gens. "
					"Any SRAM loaded with the incorrect setting will be corrupted.", this, !e.isPointer());
			ynAlertView.place(Gfx::viewportRect());
			modalView = &ynAlertView;
		}
	} bigEndianSram;

	struct RegionMenuItem : public MultiChoiceMenuItem
	{
		void init()
		{
			static const char *str[] =
			{
				"Auto", "USA", "Europe", "Japan"
			};
			int setting = 0;
			if(config.region_detect < 4)
			{
				setting = config.region_detect;
			}

			MultiChoiceMenuItem::init("Game Region", str, setting, sizeofArray(str));
		}

		void select(View *view, const InputEvent &e)
		{
			multiChoiceView.init(this, !e.isPointer());
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		void doSet(int val)
		{
			config.region_detect = val;
		}
	} region;

	MenuItem *item[24];

public:

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		OptionView::loadAudioItems(item, items);
		smsFM.init(); item[items++] = &smsFM;
	}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		sixButtonPad.init(option6BtnPad); item[items++] = &sixButtonPad;
		multitap.init(usingMultiTap); item[items++] = &multitap;
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		bigEndianSram.init(); item[items++] = &bigEndianSram;
		region.init(); item[items++] = &region;
	}

	void init(uint idx, bool highlightFirst)
	{
		uint i = 0;
		switch(idx)
		{
			bcase 0: loadVideoItems(item, i);
			bcase 1: loadAudioItems(item, i);
			bcase 2: loadInputItems(item, i);
			bcase 3: loadSystemItems(item, i);
			bcase 4: loadGUIItems(item, i);
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};
