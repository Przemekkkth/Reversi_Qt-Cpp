#ifndef GAMESCENE_H
#define GAMESCENE_H

#include <QGraphicsScene>
#include <QElapsedTimer>
#include <QPixmap>
#include <QString>
#include <QMap>
#include <QTimer>
#include <QList>
#include "utils.h"

struct KeyStatus
{
    bool m_held = false;
    bool m_released = false;
    static bool s_keyPressed;
};

struct MouseStatus
{
    float m_x = 0.0f;
    float m_y = 0.0f;
    bool m_released = false;
    bool m_pressed = false;
    static QPoint s_releasedPoint;
};


class GameScene : public QGraphicsScene
{
    Q_OBJECT
public:
    enum GameMode{
        Turn, Game, Again
    };
    explicit GameScene(QObject *parent = nullptr);

signals:

private slots:
    void loop();

private:
    void handlePlayerInput();
    void resetStatus();

    void drawBG();
    void drawBoard();
    void drawNewGameAndHintText();
    void drawInfo();
    void drawFinalScore();
    void resetBoard();
    bool isOnBoard(int x, int y);
    QPoint translateBoardToPixelCoord(int x, int y);
    QPair<QString, QString> enterPlayerTile();
    bool isValidMove(QString tile, int xstart, int ystart);
    bool isValidMove(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT], QString tile, int xstart, int ystart);
    QList<QPoint> isValidMove(QString tile, int xstart, int ystart, bool on);
    QList<QPoint> isValidMove(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT], QString tile, int xstart, int ystart, bool on);
    QList<QPoint> getValidMoves(QString tile);
    void setBoardWithValidMoves(QString tile);
    void removeHintTileFromBoard();
    QPoint getSpaceClicked();
    bool makeMove(QString tile, int xstart, int ystart, bool realMove=false);
    bool makeMove(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT], QString tile, int xstart, int ystart, bool realMove=false);
    bool isOnCorner(int x, int y);
    QPoint getComputerMove(QString computerTile);
    QMap<QString, int> getScoreOfBoard(QString dupeBoard[GAME::BOARDWIDTH][GAME::BOARDHEIGHT]);
    void newGame();
    void quitGame();

    QPixmap m_bgPixmap, m_boardPixmap;
    QString m_board[GAME::BOARDWIDTH][GAME::BOARDHEIGHT];
    QString m_playerTile, m_computerTile;
    QString m_turn;
    GameMode m_mode;
    bool m_showHints;

    KeyStatus* m_keys[256];
    MouseStatus* m_mouse;
    const int FPS = 60;
    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
    float m_deltaTime = 0.0f, m_loopTime = 0.0f;
    const float m_loopSpeed = int(1000.0f/FPS);
protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // GAMESCENE_H
