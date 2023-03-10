#include "pixmapmanager.h"

PixmapManager* PixmapManager::ptr = nullptr;

PixmapManager *PixmapManager::Instance()
{
    if(!ptr)
    {
        ptr = new PixmapManager();
    }
    return ptr;
}

QPixmap& PixmapManager::getPixmap(TextureID id)
{
    return m_textures.get(id);
}

PixmapManager::PixmapManager()
{
    m_textures.load(TextureID::BG,    ":/res/sprite/flippybackground.png");
    m_textures.load(TextureID::Black, ":/res/sprite/black.png");
    m_textures.load(TextureID::White, ":/res/sprite/white.png");
    m_textures.load(TextureID::Hint,  ":/res/sprite/hint.png");
}
