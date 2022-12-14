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
};


class GameScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GameScene(QObject *parent = nullptr);

signals:

private slots:
    void loop();

private:
    void handlePlayerInput();
    void resetStatus();

    void drawBG();
    void drawBoard();
    void resetBoard();
    bool isOnBoard(int x, int y);
    QPoint translateBoardToPixelCoord(int x, int y);
    QPair<QString, QString> enterPlayerTile();
    QPixmap m_bgPixmap, m_boardPixmap;
    QString m_board[GAME::BOARDWIDTH][GAME::BOARDHEIGHT];

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
