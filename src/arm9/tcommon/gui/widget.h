#ifndef T_WIDGET_H
#define T_WIDGET_H

#include "../common.h"
#include "gui_types.h"

class Widget {
	private:
		Rect bounds;
		bool backgroundDirty;
		bool transparent;
		bool visible;

	public:
		Screen* const screen;
		u16* background;

		u16* image; //The image source for the background image
		u16 imageW, imageH;
		s16 imageU, imageV;

		Texture* texture;
		Rect textureUV; //Texture rect for the foreground image
		s32 vw, vh;

		Widget(Screen* screen);
		virtual ~Widget();

		//Activate is called when the screen the widget is in becomes active
		void Activate();
		virtual void OnActivated(); //Override this one

		//FlushChanges is called when the user goes to the next screen.
		//You should silently save any lingering changes.
		void FlushChanges();
		virtual void OnFlushChanges(); //Override this one

		//Cancel is called when the user goes to a previous screen.
		void Cancel();
		virtual void OnCancel(); //Override this one

		virtual bool OnTouch(u32& down, u32& held, touchPosition touch); //Return true to consume the touch event
		virtual void Update(u32& down, u32& held, touchPosition touch);
		virtual void DrawBackground();
		virtual void DrawForeground();

		virtual bool IsBackgroundDirty();
		virtual bool IsVisible();
		bool IsTransparent();
		bool Contains(int x, int y);
		s16 GetX();
		s16 GetY();
		u16 GetWidth();
		u16 GetHeight();
		Rect GetBounds();

		void SetBackgroundDirty(bool v=true);
		void SetTransparent(bool t);
		virtual void SetVisible(bool v);
		virtual void SetBackgroundImage(u16* image, u16 imageW, u16 imageH, s16 imageU, s16 imageV);
		virtual void SetForegroundImage(Rect uv);
		virtual void SetPos(s16 x, s16 y);
		virtual void SetSize(u16 w, u16 h);
		virtual void SetBounds(s16 x, s16 y, u16 w, u16 h);
		virtual void SetBounds(Rect r);

};

#endif

