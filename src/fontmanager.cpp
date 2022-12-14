#include "fontmanager.h"
#include <QFontDatabase>

FontManager* FontManager::ptr = nullptr;

FontManager *FontManager::Instance()
{
    if(!ptr)
    {
        ptr = new FontManager();
    }
    return ptr;
}

QFont &FontManager::getFont(FontID id)
{
    return m_fonts[id];
}

FontManager::FontManager()
{
    load(FontID::FONT, ":/res/font/freesansbold.ttf", 16, false);
    load(FontID::BIGFONT, ":/res/font/freesansbold.ttf", 32, false);
}

void FontManager::load(FontID id, QString pathToFont, int size, bool italic)
{
    int fontID = QFontDatabase::addApplicationFont(pathToFont);
    QString fontFamilyName = QFontDatabase::applicationFontFamilies(fontID).at(0);
    m_fonts[id] = QFont(fontFamilyName, size, italic);
}
