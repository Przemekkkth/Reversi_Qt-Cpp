#include "gamescene.h"
#include <QApplication>
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
#include <QThread>
#include "utils.h"
#include "pixmapmanager.h"
#include "fontmanager.h"

bool KeyStatus::s_keyPressed = false;
QPoint MouseStatus::s_releasedPoint = QPoint(-1, -1);

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent),
      m_mode(GameMode::Turn),
      m_showHints(false)
{
    m_bgPixmap    = PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::BG).scaled(SCREEN::PHYSICAL_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

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
                MouseStatus::s_releasedPoint = QPoint(-1, -1);
                m_mode = GameMode::Game;
            }

        }
        else if(m_mode == GameMode::Game)
        {
            if(m_showHints)
            {
                setBoardWithValidMoves(m_playerTile);
            }
            else
            {
                removeHintTileFromBoard();
            }
            if(m_turn == GAME::PLAYER)
            {
                if(getValidMoves(m_playerTile).isEmpty() && !m_showHints)
                {
                    m_mode = GameMode::Again;
                }



                QPoint p = getSpaceClicked();
                if(p != QPoint(-1, -1) && isValidMove(m_playerTile, p.x(), p.y()))
                {
                    makeMove(m_playerTile, p.x(), p.y(), true);
                    if(getValidMoves(m_computerTile).size()){
                        m_turn = GAME::COMPUTER;
                    }
                }
            }
            else if(m_turn == GAME::COMPUTER)
            {
                if(getValidMoves(m_computerTile).isEmpty() && !m_showHints)
                {
                    m_mode = GameMode::Again;
                }
                //TIME
                QThread::sleep(1);

                QPoint bestMove = getComputerMove(m_computerTile);
                makeMove(m_computerTile, bestMove.x(), bestMove.y(), true);
                if(getValidMoves(m_playerTile).size()){
                    m_turn = GAME::PLAYER;
                }
            }
            clear();
            drawBG();
            drawBoard();
            drawInfo();
            drawNewGameAndHintText();
        }
        else if(m_mode == GameMode::Again)
        {
            clear();
            drawBG();
            drawBoard();
            drawFinalScore();
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
}

void GameScene::drawBoard()
{
    for(int x = 0; x < (GAME::BOARDWIDTH); ++x)
    {
        for(int y = 0; y < (GAME::BOARDHEIGHT); ++y)
        {
            QGraphicsRectItem* rItem = new QGraphicsRectItem();
            rItem->setPos((x * GAME::SPACESIZE) + GAME::XMARGIN, (y * GAME::SPACESIZE) + GAME::YMARGIN);
            rItem->setRect(0,0, GAME::SPACESIZE, GAME::SPACESIZE);
            if((x+y)%2)
            {
                rItem->setPen(GAME::LIGHT_GREEN);
                rItem->setBrush(GAME::LIGHT_GREEN);
            }
            else
            {
                rItem->setPen(GAME::DARK_GREEN);
                rItem->setBrush(GAME::DARK_GREEN);
            }
            addItem(rItem);
        }
    }

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
                QPixmap pixmap;
                int offset = 8;

                if(m_board[x][y] == GAME::WHITE_TILE)
                {
                    tileColor = GAME::WHITE;
                    pixmap = PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::White).scaled((int(GAME::SPACESIZE)-offset/2), (int(GAME::SPACESIZE)-offset/2));
                }
                else
                {
                    tileColor = GAME::BLACK;
                    pixmap = PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::Black).scaled((int(GAME::SPACESIZE)-offset/2), (int(GAME::SPACESIZE)-offset/2));
                }
                QGraphicsPixmapItem* pItem = new QGraphicsPixmapItem;
                pItem->setPixmap(pixmap);
                pItem->setPos(center.x() - (int(GAME::SPACESIZE)-offset/2)/2, center.y() - (int(GAME::SPACESIZE)-offset/2)/2);
                addItem(pItem);
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

                QGraphicsPixmapItem* pItem = new QGraphicsPixmapItem();
                pItem->setPos(center.x() - (GAME::SPACESIZE-offset/2)/2, center.y() - (GAME::SPACESIZE-offset/2)/2);
                pItem->setPixmap(PixmapManager::Instance()->getPixmap(PixmapManager::TextureID::Hint).scaled(GAME::SPACESIZE-offset/2, GAME::SPACESIZE-offset/2));
                addItem(pItem);
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
    if(m_showHints)
    {
        rItem1->setPen(QColor(GAME::TEXTBGCOLOR2));
        rItem1->setBrush(QColor(GAME::TEXTBGCOLOR2));
    }
    else
    {
        rItem1->setPen(QColor(Qt::red));
        rItem1->setBrush(QColor(Qt::red));
    }

    rItem1->setRect(tItem1->boundingRect());
    rItem1->setPos(tItem1->pos());
    addItem(rItem1);
    addItem(tItem1);


    ///////////////////////////New Game//////////////////////////////////////////////////////////
    if( MouseStatus::s_releasedPoint.x() > rItem0->x() &&
            MouseStatus::s_releasedPoint.x() < rItem0->x()+rItem0->boundingRect().width() &&
            MouseStatus::s_releasedPoint.y() > rItem0->y() &&
            MouseStatus::s_releasedPoint.y() < rItem0->y()+rItem1->boundingRect().height() )
    {
        newGame();
    }
    ///////////////////////////HINTS//////////////////////////////////////////////////////////
    if( MouseStatus::s_releasedPoint.x() > rItem1->x() &&
            MouseStatus::s_releasedPoint.x() < rItem1->x()+rItem1->boundingRect().width() &&
            MouseStatus::s_releasedPoint.y() > rItem1->y() &&
            MouseStatus::s_releasedPoint.y() < rItem1->y()+rItem1->boundingRect().height() )
    {
        MouseStatus::s_releasedPoint = QPoint(-1,-1);
        m_showHints = !m_showHints;
    }
}

void GameScene::drawInfo()
{
    QMap<QString, int> scoreMap = getScoreOfBoard(m_board);
    int computerScore = scoreMap[m_computerTile];
    int playerScore   = scoreMap[m_playerTile];
    QString textToRender = "Player Score: " + QString::number(playerScore) + " Computer Score " + QString::number(computerScore) + " Turn " + m_turn;
    QGraphicsSimpleTextItem* tItem = new QGraphicsSimpleTextItem();
    tItem->setFont(FontManager::Instance()->getFont(FontManager::FontID::FONT));
    tItem->setText(textToRender);
    tItem->setPen(QColor(GAME::TEXTCOLOR));
    tItem->setBrush(QColor(GAME::TEXTCOLOR));
    tItem->setPos(10, SCREEN::PHYSICAL_SIZE.height()-tItem->boundingRect().height());
    addItem(tItem);
}

void GameScene::drawFinalScore()
{
    QMap<QString, int> scoreMap = getScoreOfBoard(m_board);
    int computerScore = scoreMap[m_computerTile];
    int playerScore   = scoreMap[m_playerTile];
    QString text;
    if(computerScore > playerScore)
    {
        text = "You lost. The computer beat you by " + QString::number(computerScore-playerScore) + "points.";
    }
    else if(playerScore > computerScore)
    {
        text = "You beat the computer by " + QString::number(playerScore-computerScore) + " points! Congratulations!";
    }
    else
    {
        text = "The game was a tie!";
    }

    QGraphicsSimpleTextItem* resultItem = new QGraphicsSimpleTextItem();
    resultItem->setText(text);
    resultItem->setPen(GAME::TEXTCOLOR);
    resultItem->setBrush(GAME::TEXTCOLOR);
    resultItem->setFont(FontManager::Instance()->getFont(FontManager::FontID::FONT));
    resultItem->setPos(SCREEN::HALF_WIDTH-resultItem->boundingRect().width()/2,
                       SCREEN::HALF_HEIGHT-resultItem->boundingRect().height()/2);

    QGraphicsRectItem* resultRectItem = new QGraphicsRectItem();
    resultRectItem->setPos(resultItem->pos());
    resultRectItem->setPen(QColor(GAME::TEXTBGCOLOR1));
    resultRectItem->setBrush(QColor(GAME::TEXTBGCOLOR1));
    resultRectItem->setRect(resultItem->boundingRect());

    addItem(resultRectItem);
    addItem(resultItem);

    QGraphicsSimpleTextItem* gameAgainItem = new QGraphicsSimpleTextItem();
    gameAgainItem->setText("Game Again");
    gameAgainItem->setPen(GAME::TEXTCOLOR);
    gameAgainItem->setBrush(GAME::TEXTCOLOR);
    gameAgainItem->setFont(FontManager::Instance()->getFont(FontManager::FontID::BIGFONT));
    gameAgainItem->setPos(SCREEN::HALF_WIDTH-gameAgainItem->boundingRect().width()/2,
                       SCREEN::HALF_HEIGHT-gameAgainItem->boundingRect().height()/2+50);
    QGraphicsRectItem* gameAgainRectItem = new QGraphicsRectItem();
    gameAgainRectItem->setPos(gameAgainItem->pos());
    gameAgainRectItem->setPen(QColor(GAME::TEXTBGCOLOR1));
    gameAgainRectItem->setBrush(QColor(GAME::TEXTBGCOLOR1));
    gameAgainRectItem->setRect(gameAgainItem->boundingRect());

    addItem(gameAgainRectItem);
    addItem(gameAgainItem);

    QGraphicsSimpleTextItem* yesItem = new QGraphicsSimpleTextItem();
    yesItem->setText("YES");
    yesItem->setPen(GAME::TEXTCOLOR);
    yesItem->setBrush(GAME::TEXTCOLOR);
    yesItem->setFont(FontManager::Instance()->getFont(FontManager::FontID::BIGFONT));
    yesItem->setPos(SCREEN::HALF_WIDTH-yesItem->boundingRect().width()/2 - 60,
                       SCREEN::HALF_HEIGHT-yesItem->boundingRect().height()/2+120);
    QGraphicsRectItem* yesRectItem = new QGraphicsRectItem();
    yesRectItem->setPos(yesItem->pos());
    yesRectItem->setPen(QColor(GAME::TEXTBGCOLOR1));
    yesRectItem->setBrush(QColor(GAME::TEXTBGCOLOR1));
    yesRectItem->setRect(yesItem->boundingRect());

    addItem(yesRectItem);
    addItem(yesItem);

    QGraphicsSimpleTextItem* noItem = new QGraphicsSimpleTextItem();
    noItem->setText("NO");
    noItem->setPen(GAME::TEXTCOLOR);
    noItem->setBrush(GAME::TEXTCOLOR);
    noItem->setFont(FontManager::Instance()->getFont(FontManager::FontID::BIGFONT));
    noItem->setPos(SCREEN::HALF_WIDTH-noItem->boundingRect().width()/2 + 90,
                       SCREEN::HALF_HEIGHT-noItem->boundingRect().height()/2+120);
    QGraphicsRectItem* noRectItem = new QGraphicsRectItem();
    noRectItem->setPos(noItem->pos());
    noRectItem->setPen(QColor(GAME::TEXTBGCOLOR1));
    noRectItem->setBrush(QColor(GAME::TEXTBGCOLOR1));
    noRectItem->setRect(noItem->boundingRect());

    addItem(noRectItem);
    addItem(noItem);

    ///////////////////////YES///////////////////////
    if( MouseStatus::s_releasedPoint.x() > yesItem->x() &&
            MouseStatus::s_releasedPoint.x() < yesItem->x()+yesItem->boundingRect().width() &&
            MouseStatus::s_releasedPoint.y() > yesItem->y() &&
            MouseStatus::s_releasedPoint.y() < yesItem->y()+yesItem->boundingRect().height() )
    {
        newGame();
    }

    ///////////////////////NO///////////////////////
    if( MouseStatus::s_releasedPoint.x() > noItem->x() &&
            MouseStatus::s_releasedPoint.x() < noItem->x()+noItem->boundingRect().width() &&
            MouseStatus::s_releasedPoint.y() > noItem->y() &&
            MouseStatus::s_releasedPoint.y() < noItem->y()+noItem->boundingRect().height() )
    {
        quitGame();
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
    return isValidMove(m_board, tile, xstart, ystart);
}

bool GameScene::isValidMove(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT], QString tile, int xstart, int ystart)
{
    if(dupeBoard[xstart][ystart] != GAME::EMPTY_TILE || !isOnBoard(xstart, ystart))
    {
        return false;
    }

    dupeBoard[xstart][ystart] = tile;
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
        if(isOnBoard(x, y) && dupeBoard[x][y] == otherTile)
        {
            x += xdirection;
            y += ydirection;
            if(!isOnBoard(x, y))
            {
                continue;
            }
            while(dupeBoard[x][y] == otherTile)
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
            if(dupeBoard[x][y] == tile)
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

    dupeBoard[xstart][ystart] = GAME::EMPTY_TILE;
    if(!tilesToFilp.size())
    {
        return false;
    }
    return true;
}

QList<QPoint> GameScene::isValidMove(QString tile, int xstart, int ystart, bool on)
{
    return isValidMove(m_board, tile, xstart, ystart, on);
}

QList<QPoint> GameScene::isValidMove(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT], QString tile, int xstart, int ystart, bool on)
{
    if(dupeBoard[xstart][ystart] != GAME::EMPTY_TILE || !isOnBoard(xstart, ystart))
    {
        return QList<QPoint>();
    }

    dupeBoard[xstart][ystart] = tile;
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
        if(isOnBoard(x, y) && dupeBoard[x][y] == otherTile)
        {
            x += xdirection;
            y += ydirection;
            if(!isOnBoard(x, y))
            {
                continue;
            }
            while(dupeBoard[x][y] == otherTile)
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
            if(dupeBoard[x][y] == tile)
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

    dupeBoard[xstart][ystart] = GAME::EMPTY_TILE;
    if(!tilesToFilp.size())
    {
        return QList<QPoint>();
    }
    return tilesToFilp;
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

QPoint GameScene::getSpaceClicked()
{
    for(int x = 0; x < GAME::BOARDWIDTH; ++x)
    {
        for(int y = 0; y < GAME::BOARDHEIGHT; ++y)
        {
            if(MouseStatus::s_releasedPoint.x() > x*GAME::SPACESIZE + GAME::XMARGIN &&
                    MouseStatus::s_releasedPoint.x() < (x + 1)*GAME::SPACESIZE + GAME::XMARGIN &&
                    MouseStatus::s_releasedPoint.y() > y*GAME::SPACESIZE + GAME::YMARGIN &&
                    MouseStatus::s_releasedPoint.y() < (y + 1)*GAME::SPACESIZE + GAME::YMARGIN)
            {
                return QPoint(x,y);
            }
        }
    }
    return QPoint(-1,-1);
}

bool GameScene::makeMove(QString tile, int xstart, int ystart, bool realMove)
{
    return makeMove(m_board, tile, xstart, ystart, realMove);
}

bool GameScene::makeMove(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT], QString tile, int xstart, int ystart, bool realMove)
{
    QList<QPoint> tilesToFlip = isValidMove(dupeBoard, tile, xstart, ystart, true);

    if (tilesToFlip.isEmpty())
    {
        return false;
    }

    dupeBoard[xstart][ystart] = tile;

    if( realMove )
    {
        // animateTileChange(tilesToFlip, tile, (xstart, ystart))
    }

    foreach(QPoint p, tilesToFlip)
    {
        dupeBoard[p.x()][p.y()] = tile;
    }

    return true;
}

bool GameScene::isOnCorner(int x, int y)
{
    return (x == 0 && y == 0) || (x == GAME::BOARDWIDTH && y == 0) || (x == 0 and y == GAME::BOARDHEIGHT)
            || (x == GAME::BOARDWIDTH && y == GAME::BOARDHEIGHT);
}

QPoint GameScene::getComputerMove(QString computerTile)
{
    QList<QPoint> possibleMoves = getValidMoves(computerTile);
    std::random_shuffle(possibleMoves.begin(), possibleMoves.end());

    foreach(QPoint p, possibleMoves)
    {
        int x = p.x();
        int y = p.y();
        if(isOnCorner(x, y))
        {
            return p;
        }
    }

    int bestScore = 0;
    QPoint bestMove;
    foreach(QPoint p, possibleMoves)
    {
        QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT];
        for(int x = 0; x < GAME::BOARDWIDTH; ++x)
        {
            for(int y = 0; y < GAME::BOARDHEIGHT; ++y)
            {
                dupeBoard[x][y] = m_board[x][y];
            }
        }
        makeMove(dupeBoard, m_computerTile, p.x(), p.y());
        int score = getScoreOfBoard(dupeBoard)[m_computerTile];
        if(score > bestScore)
        {
            bestMove = QPoint(p.x(),p.y());
            bestScore = score;
        }
    }
    return bestMove;
}

QMap<QString, int> GameScene::getScoreOfBoard(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT])
{
    // Determine the score by counting the tiles.
    // first - WHITE_TILE, second - BLACK_TILE
    int xscore = 0;
    int oscore = 0;
    QMap<QString, int> retVal;
    for(int x = 0; x <  GAME::BOARDWIDTH; ++x)
    {
        for(int y = 0; y < GAME::BOARDHEIGHT; ++y)
        {
            if(dupeBoard[x][y] == GAME::WHITE_TILE)
            {
                xscore += 1;
            }
            if(dupeBoard[x][y] == GAME::BLACK_TILE)
            {
                oscore += 1;
            }
        }
    }

    retVal[GAME::WHITE_TILE] = xscore;
    retVal[GAME::BLACK_TILE] = oscore;
    return retVal;
}

void GameScene::newGame()
{
    m_mode = GameMode::Turn;
    m_showHints = false;
    resetBoard();
}

void GameScene::quitGame()
{
    QApplication::instance()->quit();
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
    MouseStatus::s_releasedPoint = QPoint(-1, -1);
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
