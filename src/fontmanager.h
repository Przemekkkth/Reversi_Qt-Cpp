#ifndef FONTMANAGER_H
#define FONTMANAGER_H
#include <QFont>
#include <QMap>

class FontManager
{
public:
    enum class FontID{
        FONT, BIGFONT
    };
    static FontManager* Instance();
    QFont& getFont(FontID id);
private:
    FontManager();
    FontManager(FontManager& other) = delete;
    void operator=(const FontManager &) = delete;
    void load(FontID id, QString pathToFont, int size = 12, bool italic = false);
    static FontManager* ptr;

    QMap<FontID, QFont> m_fonts;
};

#endif // FONTMANAGER_H
