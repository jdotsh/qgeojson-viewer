/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2018 Julian Sherollari <jdotsh@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QVariantMap>
#include <QQmlContext>
#include <QtLocation/private/qgeojson_p.h>
#include <QGeoCircle>
#include <QGeoPath>
#include <QGeoPolygon>
#include <QtLocation/private/qdeclarativegeomapitemview_p.h>
#include <QtLocation/private/qdeclarativegeomapquickitem_p.h>
#include <QtLocation/private/qdeclarativecirclemapitem_p.h>
#include <QtLocation/private/qdeclarativepolylinemapitem_p.h>
#include <QtLocation/private/qdeclarativepolygonmapitem_p.h>
#include <QtLocation/private/qdeclarativerectanglemapitem_p.h>
#include <QJsonObject>
#include <QJsonArray>


class GeoJsoner: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant model MEMBER m_importedGeoJson NOTIFY modelChanged)

public:
    GeoJsoner(QObject *parent = nullptr) : QObject(parent)
    {

    }

public slots:

    Q_INVOKABLE bool load(QUrl url)
    {
        // Reading GeoJSON file
        QFile loadFile(url.toLocalFile());
        if (!loadFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Error while opening the file: " << url;
            qWarning() << loadFile.error() <<  loadFile.errorString();
            return false;
        }

        // Load the GeoJSON file using Qt's API
        QJsonParseError err;
        QJsonDocument loadDoc(QJsonDocument::fromJson(loadFile.readAll(), &err));
        if (err.error) {
             qWarning() << "Error parsing while importing the JSON document:\n" << err.errorString();
             return false;
        }

        // Import geographic data to a QVariantList
        QVariantList modelList = QGeoJson::importGeoJson(loadDoc);

        QString prettyPrint = QGeoJson::toString(modelList);
        qDebug().noquote() << prettyPrint;
        QFile jsonFile("/tmp/prettyModelList.txt");
        jsonFile.open(QFile::WriteOnly);
        jsonFile.write(prettyPrint.toUtf8());
        jsonFile.close();


        m_importedGeoJson =  modelList;
        emit modelChanged();
        return true;
    }

    // Used by the MapItemView Extractor to identify a Feature
    static bool hasProperties(QQuickItem *item)
    {
        QVariant props = item->property("props");
        return !props.isNull();
    }

    static bool isFeatureCollection(QQuickItem *item)
    {
        QVariant geoJsonType = item->property("geojsonType");
        return geoJsonType.toString() == QStringLiteral("FeatureCollection");
    }

    static bool isGeoJsonEntry(QQuickItem *item)
    {
        QVariant geoJsonType = item->property("geojsonType");
        return !geoJsonType.toString().isEmpty();
    }

    QVariantMap toVariant(QDeclarativePolygonMapItem *mapPolygon)
    {
        QVariantMap ls;
        ls["type"] = "Polygon";
        ls["data"] = QVariant::fromValue(mapPolygon->geoShape());
        if (hasProperties(mapPolygon))
            ls["properties"] = mapPolygon->property("props").toMap();
        return ls;
    }
    QVariantMap toVariant(QDeclarativePolylineMapItem *mapPolyline)
    {
        QVariantMap ls;
        ls["type"] = "LineString";
        ls["data"] = QVariant::fromValue(mapPolyline->geoShape());
        if (hasProperties(mapPolyline))
            ls["properties"] = mapPolyline->property("props").toMap();
        return ls;
    }
    QVariantMap toVariant(QDeclarativeCircleMapItem *mapCircle)
    {
        QVariantMap pt;
        pt["type"] = "Point";
        pt["data"] = QVariant::fromValue(mapCircle->geoShape());
        if (hasProperties(mapCircle))
            pt["properties"] = mapCircle->property("props").toMap();
        return pt;
    }

    QVariantMap toVariant(QDeclarativeGeoMapItemView *mapItemView)
    {
        bool featureCollecton = isFeatureCollection(mapItemView);
        // if not a feature collection, this must be a geometry collection,
        // or a multilinestring/multipoint/multipolygon.
        // To disambiguate, one could check for heterogeneity.
        // For simplicity, in this example, we expect the property "geojsonType" to be injected in the mapItemView
        // by the delegate, and to be correct.

        QString nodeType = mapItemView->property("geojsonType").toString();
        QVariantMap root;
        if (!nodeType.isEmpty())  // empty nodeType can happen only for the root MIV
            root["type"] = nodeType;
        if (hasProperties(mapItemView)) // Features are converted to regular types w properties.
            root["properties"] = mapItemView->property("props").toMap();

        QVariantList features;
        const QList<QQuickItem *> &quickChildren = mapItemView->childItems();
        for (auto kid : quickChildren) {
            QVariant entry;
            if (QDeclarativeGeoMapItemView *miv = qobject_cast<QDeclarativeGeoMapItemView *>(kid)) {
                // handle nested miv
                entry = toVariant(miv);
                if (nodeType.isEmpty()) // dirty hack to handle (=skip) the first MIV used to process the fictitious list with 1 element
                    return entry.toMap();
            } else if (QDeclarativePolylineMapItem *polyline = qobject_cast<QDeclarativePolylineMapItem *>(kid)) {
                entry = toVariant(polyline);
            } else if (QDeclarativePolygonMapItem *polygon = qobject_cast<QDeclarativePolygonMapItem *>(kid)) {
                entry = toVariant(polygon);
            } else if (QDeclarativeCircleMapItem *circle = qobject_cast<QDeclarativeCircleMapItem *>(kid)) {
                entry = toVariant(circle); // if GeoJSON Point type is visualized in other ways, handle those types here instead.
            }
            features.append(entry);
        }
        root["data"] = features;
        return root;
    }
    Q_INVOKABLE QVariantList toGeoJson(QDeclarativeGeoMapItemView *mapItemView)
    {
        QVariantList res;
        QDeclarativeGeoMapItemView *root = mapItemView;
        QVariantMap miv = toVariant(root);
        if (!miv.isEmpty())
            res.append(miv);
//        printQVariant(res);
        return res;
    }

    Q_INVOKABLE void dumpGeoJSON(QVariantList geoJson, QUrl url)
    {
        QJsonDocument json = QGeoJson::exportGeoJson(geoJson);
        QFile jsonFile(url.toLocalFile());
        jsonFile.open(QFile::WriteOnly);
        jsonFile.write(json.toJson());
        jsonFile.close();
    }

signals:
    void modelChanged();

public:
    QVariant m_importedGeoJson;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    // Switch to QML app
    QQmlApplicationEngine engine;

    // engine.rootContext()->setContextProperty("GeoJsonr", new GeoJsoner(&engine));

    qmlRegisterType<GeoJsoner>("Qt.GeoJson", 1, 0, "GeoJsoner");
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
