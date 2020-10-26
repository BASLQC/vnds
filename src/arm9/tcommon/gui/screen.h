#ifndef T_SCREEN_H
#define T_SCREEN_H

#include <list>
#include "../common.h"
#include "gui_types.h"

class Screen {
	private:
		std::list<Widget*> widgets;
		bool active;
		bool backgroundDirty;

	protected:
		u16* textureImage;

		int ListWidgets(Widget** out);
		int ListWidgetsReverse(Widget** out);

	public:
		GUI* gui;
		Texture* texture;
		u16* background;
		u16* backgroundImage;

		Screen(GUI* gui);
		virtual ~Screen();

		virtual void Activate();
		virtual void Deactivate();
		virtual void Cancel();

		virtual void Update(u32& keysDown, u32& keysHeld, touchPosition touch);
		virtual void DrawBackground();
		virtual void DrawForeground();
		virtual void DrawTopScreen();

		bool Draw();
		void AddWidget(Widget* w);
		void RemoveWidget(Widget* w);

		void SetBackgroundDirty(bool v=true);
		void SetBackgroundImage(u16* backgroundImage); //Has to be 256x192
		void SetTexture(u16* textureImage);

};

#endif
