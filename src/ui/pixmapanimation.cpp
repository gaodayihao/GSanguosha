#include "pixmapanimation.h"
#include <QPainter>
#include <QPixmapCache>
#include <QDir>

PixmapAnimation::PixmapAnimation(QGraphicsScene *scene) :
    QGraphicsItem(0,scene)
{
}

void PixmapAnimation::advance(int phase)
{
    if(phase)current++;
    if(current>=frames.size())
    {
        current = 0;
        emit finished();
    }
    update();
}

void PixmapAnimation::setPath(const QString &path)
{
    frames.clear();
    current = 0;

    int i = 0;
    QString pic_path = QString("%1%2%3").arg(path).arg(i++).arg(".png");
    do{
        frames << GetFrameFromCache(pic_path);
        pic_path = QString("%1%2%3").arg(path).arg(i++).arg(".png");
    }while(!GetFrameFromCache(pic_path).isNull());
}

void PixmapAnimation::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->drawPixmap(0,0,frames.at(current));
}

QRectF PixmapAnimation::boundingRect() const
{
    return frames.at(current).rect();
}

bool PixmapAnimation::valid()
{
    return !frames.isEmpty();
}

void PixmapAnimation::timerEvent(QTimerEvent *)
{
    advance(1);
}

void PixmapAnimation::start(bool permanent,int interval)
{
    startTimer(interval);
    if(!permanent)connect(this,SIGNAL(finished()),this,SLOT(deleteLater()));
}

PixmapAnimation* PixmapAnimation::GetPixmapAnimation(QGraphicsObject *parent, const QString &emotion, int x,int y)
{
    PixmapAnimation *pma = new PixmapAnimation();
    pma->setPath(QString("image/system/emotion/%1/").arg(emotion));
    if(pma->valid())
    {

        if(emotion == "no-success")
        {
            pma->moveBy(pma->boundingRect().width()*0.15,
                        pma->boundingRect().height()*0.15);
            pma->setScale(0.7);
        }

        else if(emotion.contains("double_sword"))
        {
            pma->moveBy(13, -85);
            pma->setScale(1.3);
        }

        else if(emotion.contains("fan") || emotion.contains("guding_blade"))
        {
            pma->moveBy(-30, -80);
            pma->setScale(1.3);
        }

        else if(emotion.contains("/spear"))
        {
            pma->moveBy(-90, -80);
            pma->setScale(1.3);
        }

        pma->moveBy((parent->boundingRect().width() - pma->boundingRect().width())/2 + x,
                    (parent->boundingRect().height() - pma->boundingRect().height())/2 + y);

        pma->setParentItem(parent);
        pma->setZValue(2.5);
        pma->startTimer(50);
        connect(pma,SIGNAL(finished()),pma,SLOT(deleteLater()));
        return pma;
    }
    else
    {
        delete pma;
        return NULL;
    }
}

QPixmap PixmapAnimation::GetFrameFromCache(const QString &filename){
    QPixmap pixmap;
    if(!QPixmapCache::find(filename, &pixmap)){
        pixmap.load(filename);
        if(!pixmap.isNull())
            QPixmapCache::insert(filename, pixmap);
    }

    return pixmap;
}

int PixmapAnimation::GetFrameCount(const QString &emotion){
    QString path = QString("image/system/emotion/%1/").arg(emotion);
    QDir dir(path);
    return dir.entryList(QDir::Files | QDir::NoDotAndDotDot).count();
}
