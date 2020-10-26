#include "dssprite.h"

DSSprite::DSSprite(SpriteEntry* sprite, Rect uv, s16 x, s16 y) {
    this->sprite = sprite;

    sprite->attribute[0] = ATTR0_BMP | ATTR0_DISABLED;
    sprite->attribute[1] = 0;
    sprite->attribute[2] = ATTR2_ALPHA(7);

    SetUV(uv);
    SetPos(x, y);
}
DSSprite::~DSSprite() {
    sprite->attribute[0] = ATTR0_DISABLED;
}

void DSSprite::GetSizeAttributes(ObjSize* as, ObjShape* shape, u8 w, u8 h) {
    *as = OBJSIZE_8;
    if (w == h) {
        *shape = OBJSHAPE_SQUARE;
        if (w >= 64)      *as = OBJSIZE_64;
        else if (w >= 32) *as = OBJSIZE_32;
        else if (w >= 16) *as = OBJSIZE_16;
    } else if (w > h) {
        *shape = OBJSHAPE_WIDE;
        if (h >= 32)      *as = OBJSIZE_64;
        else if (h >= 16) *as = OBJSIZE_32;
        else if (w >= 32) *as = OBJSIZE_16;
    } else {
        *shape = OBJSHAPE_TALL;
        if (w >= 32)      *as = OBJSIZE_64;
        else if (w >= 16) *as = OBJSIZE_32;
        else if (h >= 32) *as = OBJSIZE_16;
    }
}

s16 DSSprite::GetX() {
    return x;
}
s16 DSSprite::GetY() {
    return y;
}

void DSSprite::SetPos(s16 x, s16 y) {
    this->x = x;
    this->y = y;

    if (x > -w && x < SCREEN_WIDTH && y > -h && y < SCREEN_HEIGHT) {
        sprite->attribute[0] = ((sprite->attribute[0] & 0xFF00) & ~ATTR0_DISABLED) | (y & 0x00FF);
        sprite->attribute[1] = (sprite->attribute[1] & 0xFE00) | (x & 0x01FF);
    } else {
        sprite->attribute[0] |= ATTR0_DISABLED;
    }
}

void DSSprite::SetVisible(bool v) {
    sprite->isHidden = !v;
}

void DSSprite::SetUV(Rect uv) {
    w = uv.w;
    h = uv.h;

    ObjSize as;
    ObjShape shape;
    GetSizeAttributes(&as, &shape, w, h);

    sprite->shape = shape;
    sprite->size = as;
    sprite->gfxIndex = ((uv.y>>3)<<5) | (uv.x>>3);
}
