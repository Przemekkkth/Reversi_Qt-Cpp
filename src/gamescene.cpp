#include "gamescene.h"
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QDir>
#include <QGraphicsLineItem>
#include "utils.h"
#include "pixmapmanager.h"
#include "fontmanager.h"

bool KeyStatus::s_keyPressed = false;

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent)
{
    m_bgPixmap    = PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::BG).scaled(SCREEN::PHYSICAL_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_boardPixmap = PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::Board).scaled(GAME::BOARDWIDTH *
                                                                                                 GAME::SPACESIZE, GAME::BOARDHEIGHT * GAME::SPACESIZE);
    drawBG();
    resetBoard();
    drawBoard();
    enterPlayerTile();

    for(int i = 0; i < 256; ++i)
    {
        m_keys[i] = new KeyStatus();
    }
    m_mouse = new MouseStatus();
    setSceneRect(0,0, SCREEN::PHYSICAL_SIZE.width(), SCREEN::PHYSICAL_SIZE.height());
    connect(&m_timer, &QTimer::timeout, this, &GameScene::loop);
    m_timer.start(int(1000.0f/FPS));
    m_elapsedTimer.start();
    srand(time(0));
}

void GameScene::loop()
{
    m_deltaTime = m_elapsedTimer.elapsed();
    m_elapsedTimer.restart();

    m_loopTime += m_deltaTime;
    if( m_loopTime > m_loopSpeed)
    {
        m_loopTime -= m_loopSpeed;

        handlePlayerInput();
        resetStatus();
    }
}

void GameScene::handlePlayerInput()
{
}

void GameScene::resetStatus()
{
    for(int i = 0; i < 256; ++i)
    {
        m_keys[i]->m_released = false;
    }
    m_mouse->m_released = false;
}

void GameScene::drawBG()
{
    QGraphicsPixmapItem* bgItem = new QGraphicsPixmapItem();
    bgItem->setPixmap(m_bgPixmap);
    addItem(bgItem);

    QGraphicsPixmapItem* boardItem = new QGraphicsPixmapItem();
    boardItem->setPos(GAME::XMARGIN, GAME::YMARGIN);
    boardItem->setPixmap(m_boardPixmap);
    addItem(boardItem);
}

void GameScene::drawBoard()
{
    for(int x = 0; x < (GAME::BOARDWIDTH+1); ++x)
    {
        int startx = (x * GAME::SPACESIZE) + GAME::XMARGIN;
        int starty = GAME::YMARGIN;
        int endx = (x * GAME::SPACESIZE) + GAME::XMARGIN;
        int endy = GAME::YMARGIN + (GAME::BOARDHEIGHT * GAME::SPACESIZE);
        QGraphicsLineItem *lItem = new QGraphicsLineItem();
        lItem->setPen(QColor(GAME::GRIDLINECOLOR));
        lItem->setLine(startx, starty, endx, endy);
        addItem(lItem);
    }

    for(int y = 0; y < (GAME::BOARDHEIGHT+1); ++y)
    {
        int startx = GAME::XMARGIN;
        int starty = (y * GAME::SPACESIZE) + GAME::YMARGIN;
        int endx = GAME::XMARGIN + (GAME::BOARDWIDTH * GAME::SPACESIZE);
        int endy = (y * GAME::SPACESIZE) + GAME::YMARGIN;
        QGraphicsLineItem *lItem = new QGraphicsLineItem();
        lItem->setPen(QColor(GAME::GRIDLINECOLOR));
        lItem->setLine(startx, starty, endx, endy);
        addItem(lItem);
    }

    for(int x = 0; x < GAME::BOARDWIDTH; ++x)
    {
        for(int y = 0; y < GAME::BOARDHEIGHT; ++y)
        {
            QPoint center = translateBoardToPixelCoord(x, y);
            if(m_board[x][y] == GAME::WHITE_TILE || m_board[x][y] == GAME::BLACK_TILE)
            {
                QColor tileColor;

                if(m_board[x][y] == GAME::WHITE_TILE)
                {
                    tileColor = GAME::WHITE;
                }
                else
                {
                    tileColor = GAME::BLACK;
                }
                QGraphicsEllipseItem* eItem = new QGraphicsEllipseItem();
                eItem->setPen(QColor(tileColor));
                eItem->setBrush(QColor(tileColor));
                int offset = 8;
                eItem->setRect(-(int(GAME::SPACESIZE)-offset)/2,-(int(GAME::SPACESIZE)-offset)/2 , int(GAME::SPACESIZE)-offset, int(GAME::SPACESIZE)-offset);
                eItem->setPos(center.x(), center.y());
                addItem(eItem);

            }
            if(m_board[x][y] == GAME::HINT_TILE)
            {
                QGraphicsRectItem *rItem = new QGraphicsRectItem();
                rItem->setPen(QColor(GAME::HINTCOLOR));
                rItem->setBrush(QColor(GAME::HINTCOLOR));
                int offset = 8;
                rItem->setRect(-offset/2, -offset/2, offset, offset);
                rItem->setPos(center.x(), center.y());
                addItem(rItem);
            }
        }
    }
}

void GameScene::resetBoard()
{
    for(int x = 0; x < GAME::BOARDWIDTH; ++x)
    {
        for(int y = 0; y < GAME::BOARDHEIGHT; ++y)
        {
            m_board[x][y] = GAME::EMPTY_TILE;
        }
    }
    m_board[3][3] = GAME::WHITE_TILE;
    m_board[3][4] = GAME::BLACK_TILE;
    m_board[4][3] = GAME::BLACK_TILE;
    m_board[4][4] = GAME::WHITE_TILE;
}

bool GameScene::isOnBoard(int x, int y)
{
    return x >= 0 && x < GAME::BOARDWIDTH && y >= 0 && y < GAME::BOARDHEIGHT;
}

QPoint GameScene::translateBoardToPixelCoord(int x, int y)
{
    return QPoint(GAME::XMARGIN + x * GAME::SPACESIZE + int(GAME::SPACESIZE / 2),
                  GAME::YMARGIN + y * GAME::SPACESIZE + int(GAME::SPACESIZE / 2));
}

QPair<QString, QString> GameScene::enterPlayerTile()
{
//////////////////////////////////////////
    QGraphicsSimpleTextItem* tItem0 = new QGraphicsSimpleTextItem();
    tItem0->setText("Do you want to be white or black?");
    tItem0->setPen(QColor(GAME::TEXTCOLOR));
    tItem0->setBrush(QColor(GAME::TEXTCOLOR));
    tItem0->setPos(SCREEN::HALF_WIDTH-tItem0->boundingRect().width()/2,
                  SCREEN::HALF_HEIGHT-tItem0->boundingRect().height()/2);
    tItem0->setFont(FontManager::Instance()->getFont(FontManager::FontID::FONT));

    QGraphicsRectItem* rItem0 = new QGraphicsRectItem();
    rItem0->setPos(tItem0->pos());
    rItem0->setRect(tItem0->boundingRect());
    rItem0->setPen(QColor(GAME::TEXTBGCOLOR1));
    rItem0->setBrush(QColor(GAME::TEXTBGCOLOR1));
    addItem(rItem0);
    addItem(tItem0);
///////////////////////////////////////////
    QGraphicsSimpleTextItem* tItem1 = new QGraphicsSimpleTextItem();
    tItem1->setText("White");
    tItem1->setPen(QColor(GAME::TEXTCOLOR));
    tItem1->setBrush(QColor(GAME::TEXTCOLOR));
    tItem1->setPos(SCREEN::HALF_WIDTH-tItem1->boundingRect().width()/2 - 60,
                  SCREEN::HALF_HEIGHT-tItem1->boundingRect().height()/2 + 40);
    tItem1->setFont(FontManager::Instance()->getFont(FontManager::FontID::BIGFONT));
    QGraphicsRectItem* rItem1 = new QGraphicsRectItem();
    rItem1->setPos(tItem1->pos());
    rItem1->setRect(tItem1->boundingRect());
    rItem1->setPen(QColor(GAME::TEXTBGCOLOR1));
    rItem1->setBrush(QColor(GAME::TEXTBGCOLOR1));
    addItem(rItem1);
    addItem(tItem1);
///////////////////////////////////////////
    QGraphicsSimpleTextItem* tItem2 = new QGraphicsSimpleTextItem();
    tItem2->setText("Black");
    tItem2->setPen(QColor(GAME::TEXTCOLOR));
    tItem2->setBrush(QColor(GAME::TEXTCOLOR));
    tItem2->setPos(SCREEN::HALF_WIDTH-tItem2->boundingRect().width()/2 + 60,
                   SCREEN::HALF_HEIGHT-tItem2->boundingRect().height()/2 + 40);
    tItem2->setFont(FontManager::Instance()->getFont(FontManager::FontID::BIGFONT));
    QGraphicsRectItem* rItem2 = new QGraphicsRectItem();
    rItem2->setPos(tItem2->pos());
    rItem2->setRect(tItem2->boundingRect());
    rItem2->setPen(QColor(GAME::TEXTBGCOLOR1));
    rItem2->setBrush(QColor(GAME::TEXTBGCOLOR1));
    addItem(rItem2);
    addItem(tItem2);

    return QPair<QString, QString>();
}

void GameScene::keyPressEvent(QKeyEvent *event)
{
    if(KEYBOARD::KeysMapper.contains(event->key()))
    {
        KeyStatus::s_keyPressed = true;
        m_keys[KEYBOARD::KeysMapper[event->key()]]->m_held = true;
    }
    QGraphicsScene::keyPressEvent(event);
}

void GameScene::keyReleaseEvent(QKeyEvent *event)
{
    if(!event->isAutoRepeat())
    {
        if(KEYBOARD::KeysMapper.contains(event->key()))
        {
            KeyStatus::s_keyPressed = false;
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_held = false;
            m_keys[KEYBOARD::KeysMapper[event->key()]]->m_released = true;
        }
    }

    QGraphicsScene::keyReleaseEvent(event);
}

void GameScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_mouse->m_x = event->scenePos().x();
    m_mouse->m_y = event->scenePos().y();
    m_mouse->m_pressed = true;
    QGraphicsScene::mousePressEvent(event);
}

void GameScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    m_mouse->m_x = event->scenePos().x();
    m_mouse->m_y = event->scenePos().y();
    QGraphicsScene::mouseMoveEvent(event);
}

void GameScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_mouse->m_x = event->scenePos().x();
    m_mouse->m_y = event->scenePos().y();
    m_mouse->m_pressed = false;
    m_mouse->m_released = true;
    QGraphicsScene::mouseReleaseEvent(event);
}
