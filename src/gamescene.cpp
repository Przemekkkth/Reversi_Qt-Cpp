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
QPoint MouseStatus::s_releasedPoint = QPoint(-1, -1);

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent),
      m_mode(GameMode::Turn),
      m_showHints(true)
{
    m_bgPixmap    = PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::BG).scaled(SCREEN::PHYSICAL_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_boardPixmap = PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::Board).scaled(GAME::BOARDWIDTH *
                                                                                                 GAME::SPACESIZE, GAME::BOARDHEIGHT * GAME::SPACESIZE);

    resetBoard();

    drawBG();
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

        if(m_mode == GameMode::Turn)
        {
            clear();
            drawBG();
            drawBoard();
            QPair<QString, QString> tilePair = enterPlayerTile();
            m_playerTile = tilePair.first;
            m_computerTile = tilePair.second;
            if(rand()%2)
            {
               m_turn = GAME::PLAYER;
            }
            else
            {
                m_turn = GAME::COMPUTER;
            }

            m_turn = GAME::PLAYER;
            if(m_playerTile != QString() || m_computerTile != QString())
            {
                qDebug() << "Player: " << m_playerTile << " Computer: " << m_computerTile;
                qDebug() << "turn" << m_turn;
                m_mode = GameMode::Game;
            }

        }
        else{
            if(m_turn == GAME::PLAYER)
            {
                if(getValidMoves(m_playerTile).isEmpty())
                {
                    m_mode = GameMode::Again;
                }

                if(m_showHints)
                {
                    setBoardWithValidMoves(m_playerTile);
                }
                else
                {

                }

            }

            clear();
            drawBG();
            drawBoard();
            drawNewGameAndHintText();
        }

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

void GameScene::drawNewGameAndHintText()
{
    QGraphicsSimpleTextItem *tItem0 = new QGraphicsSimpleTextItem();
    tItem0->setFont(FontManager::Instance()->getFont(FontManager::FontID::FONT));
    tItem0->setPen(QColor(GAME::TEXTCOLOR));
    tItem0->setBrush(QColor(GAME::TEXTCOLOR));
    tItem0->setText("New Game");
    tItem0->setPos(SCREEN::PHYSICAL_SIZE.width()-tItem0->boundingRect().width()-8,
                   10);

    QGraphicsRectItem *rItem0 = new QGraphicsRectItem();
    rItem0->setPen(QColor(GAME::TEXTBGCOLOR2));
    rItem0->setBrush(QColor(GAME::TEXTBGCOLOR2));
    rItem0->setRect(tItem0->boundingRect());
    rItem0->setPos(tItem0->pos());
    addItem(rItem0);
    addItem(tItem0);


    QGraphicsSimpleTextItem *tItem1 = new QGraphicsSimpleTextItem();
    tItem1->setFont(FontManager::Instance()->getFont(FontManager::FontID::FONT));
    tItem1->setPen(QColor(GAME::TEXTCOLOR));
    tItem1->setBrush(QColor(GAME::TEXTCOLOR));
    tItem1->setText("Hints");
    tItem1->setPos(SCREEN::PHYSICAL_SIZE.width()-tItem1->boundingRect().width()-8,
                   50);

    QGraphicsRectItem *rItem1 = new QGraphicsRectItem();
    rItem1->setPen(QColor(GAME::TEXTBGCOLOR2));
    rItem1->setBrush(QColor(GAME::TEXTBGCOLOR2));
    rItem1->setRect(tItem1->boundingRect());
    rItem1->setPos(tItem1->pos());
    addItem(rItem1);
    addItem(tItem1);
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
    tItem0->setPos(SCREEN::HALF_WIDTH-tItem0->boundingRect().width()/1.5f,
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
    tItem1->setPos(SCREEN::HALF_WIDTH-tItem1->boundingRect().width()/1.5f - 60,
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
    tItem2->setPos(SCREEN::HALF_WIDTH-tItem2->boundingRect().width()/1.5f + 60,
                   SCREEN::HALF_HEIGHT-tItem2->boundingRect().height()/2 + 40);
    tItem2->setFont(FontManager::Instance()->getFont(FontManager::FontID::BIGFONT));
    QGraphicsRectItem* rItem2 = new QGraphicsRectItem();
    rItem2->setPos(tItem2->pos());
    rItem2->setRect(tItem2->boundingRect());
    rItem2->setPen(QColor(GAME::TEXTBGCOLOR1));
    rItem2->setBrush(QColor(GAME::TEXTBGCOLOR1));
    addItem(rItem2);
    addItem(tItem2);
/////////////////////////////////////////////////
    if( MouseStatus::s_releasedPoint.x() > rItem1->x() &&
        MouseStatus::s_releasedPoint.x() < rItem1->x()+rItem1->boundingRect().width() &&
        MouseStatus::s_releasedPoint.y() > rItem1->y() &&
        MouseStatus::s_releasedPoint.y() < rItem1->y()+rItem1->boundingRect().height() )
    {
        QPair<QString, QString> pair;
        pair.first = GAME::WHITE_TILE;
        pair.second = GAME::BLACK_TILE;
        return pair;
    }
    else if(MouseStatus::s_releasedPoint.x() > tItem2->x() &&
            MouseStatus::s_releasedPoint.x() < tItem2->x()+tItem2->boundingRect().width() &&
            MouseStatus::s_releasedPoint.y() > tItem2->y() &&
            MouseStatus::s_releasedPoint.y() < tItem2->y()+tItem2->boundingRect().height())
    {
        QPair<QString, QString> pair;
        pair.first = GAME::BLACK_TILE;
        pair.second = GAME::WHITE_TILE;
        return pair;
    }
    return QPair<QString, QString>();
}

bool GameScene::isValidMove(QString tile, int xstart, int ystart)
{
    if(m_board[xstart][ystart] != GAME::EMPTY_TILE || !isOnBoard(xstart, ystart))
    {
        return false;
    }

    m_board[xstart][ystart] = tile;
    QString otherTile;
    if(tile == GAME::WHITE_TILE)
    {
        otherTile = GAME::BLACK_TILE;
    }
    else
    {
        otherTile = GAME::WHITE_TILE;
    }

    QList<QPoint> tilesToFilp;
    QList<QPoint> eightDirection;
    eightDirection.push_back(QPoint(0,  1));
    eightDirection.push_back(QPoint(1,  1));
    eightDirection.push_back(QPoint(1,  0));
    eightDirection.push_back(QPoint(1, -1));
    eightDirection.push_back(QPoint(0, -1));
    eightDirection.push_back(QPoint(-1,-1));
    eightDirection.push_back(QPoint(-1, 0));
    eightDirection.push_back(QPoint(-1, 1));

    foreach(QPoint p, eightDirection)
    {
        int xdirection = p.x();
        int ydirection = p.y();
        int x = xstart;
        int y = ystart;
        x += xdirection;
        y += ydirection;
        if(isOnBoard(x, y) && m_board[x][y] == otherTile)
        {
            x += xdirection;
            y += ydirection;
            if(!isOnBoard(x, y))
            {
                continue;
            }
            while(m_board[x][y] == otherTile)
            {
                x += xdirection;
                y += ydirection;
                if(!isOnBoard(x, y))
                {
                    break;
                }
            }
            if(!isOnBoard(x, y))
            {
                continue;
            }
            if(m_board[x][y] == tile)
            {
                while(true)
                {
                    x -= xdirection;
                    y -= ydirection;
                    if(x == xstart && y == ystart)
                    {
                         break;
                    }
                    tilesToFilp.append(QPoint(x,y));
                }
            }
        }
    }

    m_board[xstart][ystart] = GAME::EMPTY_TILE;
    if(!tilesToFilp.size())
    {
        return false;
    }
    return true;
}

QList<QPoint> GameScene::getValidMoves(QString tile)
{
    QList<QPoint> validMoves;
    for(int x = 0; x < GAME::BOARDWIDTH; ++x)
    {
        for(int y = 0; y < GAME::BOARDHEIGHT; ++y)
        {
            if(isValidMove(tile, x, y))
            {
                validMoves.append(QPoint(x, y));
            }
        }
    }
       // exit(0);
    return validMoves;
}

void GameScene::setBoardWithValidMoves(QString tile)
{
    foreach(QPoint p, getValidMoves(tile))
    {
        int x = p.x();
        int y = p.y();
        m_board[x][y] = GAME::HINT_TILE;
    }
}

void GameScene::removeHintTileFromBoard()
{
    for(int x = 0; x < GAME::BOARDWIDTH; ++x)
    {
        for(int y = 0; y < GAME::BOARDHEIGHT; ++y)
        {
            if(m_board[x][y] == GAME::HINT_TILE)
            {
                m_board[x][y] = GAME::EMPTY_TILE;
            }
        }
    }
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
    MouseStatus::s_releasedPoint = event->scenePos().toPoint();
    m_mouse->m_pressed = false;
    m_mouse->m_released = true;
    QGraphicsScene::mouseReleaseEvent(event);
}
