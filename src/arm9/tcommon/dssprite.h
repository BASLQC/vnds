#ifndef T_DSSPRITE_H
#define T_DSSPRITE_H

#include "common.h"

class DSSprite {
    private:
        SpriteEntry* sprite;
        s16 x, y;
        u8 w, h;

        void GetSizeAttributes(ObjSize* as, ObjShape* shape, u8 w, u8 h);

    public:
        DSSprite(SpriteEntry* sprite, Rect uv, s16 x=256, s16 y=0);
        virtual ~DSSprite();

        s16 GetX();
        s16 GetY();
        void SetPos(s16 x, s16 y);
        void SetVisible(bool visible);
        void SetUV(Rect uv);
};

#endif
