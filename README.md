# Simple app to test the QGeoJson member functions.

## **Add GeoJSON interoperability to QtLocation**

#### **Breve Descrizione:**

Lo scopo del progetto era di rendere la libreria QtLocation del framework Qt interoperabile con lo standard GeoJSON. Questa interoperabilità è stata ottenuta operativamente attraveso 3 azioni principali.

1. **Feature parity:**

Il conseguimento della equivalenza delle geometrie GeoJSON e quelle presenti in QtLocation.

2. **Importing:**

Realizzazione di un parser che partendo da un documento GeoJSON restituisca una struttura dati opportunamente modellata, contenete le geometrie QtLocation equivalenti a quelle presenti nel documento GeoJSON di partenza. Creazione di un esempio che illustri come visualizzre su una mappa le geometrie importate utilizzando il QML.

3. **Exporting:**

Realizzazione di "extractor" che, partendo da una visualizzazione di geometrie su una mappa restituisca la stessa struttura dati del punto precedente. Sviluppo di un parser che partendo da quest'ultima struttura dati produca un documento GeoJSON valido.

Operativamente è stata creata una nuova classe **QGeoJson** dotata di due metodi che implementano i parser illustrati nel punto 2 e 3. Inoltre nell'ampia documentazione realizzata a dotazione della classe è stata ampiemente illustrata l'organizzazione dalla struttura dati contenente le geometrie importate (nel caso dell'importer) o le geometrie da esportare (nel caso dell'exporter). 

Recentemente è stato sviluppato un ulteriore metodo per la visualizzazione di questa struttura dati a scopo di debug.



## **Sommario:**


1. #### Le strutture Qt e analisi della parity
 
- QVariant, QVariantList e QVariantMap e QJsonDocument

- C++ e le classi QGeoShape, QGeoCircle, QGeoPath e QGeoPolygon

- Qml e le classi GeoShape, MapCircle, MapPolyLine e MapPolygon

| Geometrie Qt C++     | Geometrie Qt QML    |
| :-------------------:| :-----------------: |
| QGeoCircle           | MapCircle           |
| QGeoPath             | MapPolyLine         | 
| QGeoPolygon          | MapPolygon          | 


2. #### Conseguimento della feature parity tra QtLocation e GeoJSON

- Corrispondenza tra le geometrie

- Classi Modificate per il supporto ai buchi:
  - QGeoPolygon
  - MapPolygon

- Metodi ed attributi aggiunti



3. #### La classe [**QGeoJson**](https://github.com/jdotsh/tesi/blob/master/source/qgeojson_p.h):

- La classe QGeoJson ha 3 metodi:

  - Un importer che accetta un QJsonDocuments e restituisce una QVriantList strtturata in un modo ben specifico:

     `static QVariantList importGeoJson (const QJsonDocument &geojsonDoc);`
     
     

  - Un exporter che accetta la QVarianList di cui sopra e restituisce un QJsonDocument:

    `static QJsonDocument exportGeoJson (const QVariantList &geojsonMap);`
    
    
    
  - Un metodo per la visualizzazione della QVariantList restituita dell'importer a scopi di debug e di implementazione del delegate:

    `static QJsonDocument prettyPrint (const QVariantList &geojsonMap);`
    
    
    
4. #### Documentazione relativa all'[implementazione](https://github.com/jdotsh/tesi/blob/master/source/qgeojson.cpp):

- **L'importer:**

  L'importer accetta un QJsonDocument dal quale viene estratto un singolo oggetto, poichè secondo l'[RFC ](https://tools.ietf.org/html/rfc7946) un documento GeoJSON è sempre costituito da un singolo oggetto JSON. L'unica convalida operata durante l'estrazione dei dati GeoJSON viene fatta utilizzando le API di Qt per la convalida di un semplice documento JSON.

  L'importer restituisce una QVariantList per mantenere la compatibilità con la ricorsione del delegate in QML. La QVariantList contiene sempre una singola QVariantMap, quest'ultima è la vera geometria importata. 

  La map contiene sempre almeno due coppie  (chiave, valore), la prima avente come chiave una QString "type" e come corrispondente valore una QString identificante gli oggetti GeoJSON definiti nell'[RFC](https://tools.ietf.org/html/rfc7946) (vedi tabella), la seconda coppia ha come chiave una QString "data" e contiene nel valore le geometrie Qt ottenute dall'importazione, e strutturate in un modo ben specifico (in particolare in modo da garantire la visualizzazione delle geometrie in una mappa in modo semplice).
  
  
|     | Oggetti GeoJSON      | Geometrie Qt         |
|:--: | :------------------- | :-------------------:|
| 1.  | Point                | QGeoCircle           |
| 2.  | LineString           | QGeoPath             |
| 3.  | Polygon              | QGeoPolygon          |
| 4.  | MultiPoint           | QVariantList         | 
| 5.  | MultiLineString      | QVariantList         |
| 6.  | MultiPolygon         | QVariantList         |
| 7.  | GeometryCollection   | QVariantList         |
| 8.  | Feature              | QVariantList         |
| 9.  | FeatureCollection    | QVariantList         |

   - Per le singole geometrie (Point, LineString, Polygon) il valore corrispondente alla chiave "data" è un QGeoShape.
   
   - Per le multiple geometrie omogenee (MultiPoint, MultiLineString, MultiPolygon) il valore corrispondente alla chiave "data" è una QVariantList. Ogni elemento della QVariantList è una QVariantMap delle singole geometrie, ovviamente tutte dello stesso tipo.
  
  - La GeometryCollection è una composizione eterogenea di gemetrie. Il valore corrispondente alla chiave "data" è una QVariantList popolata da QVariantMaps delle precedenti sei geometrie e anche di GeometryCollection stessa (non consigliato nel RFC).
  
  - L'oggetto Feauture include una delle precedenti entità. È strutturato esattamente come una delle 7 precedenti geometrie più il membro "properties". Il valore di questo membro è una QVariantMap coppie chiave/valore entrambe di tipo QString. L'unico modo per distinguere una Feature da una geometria è controllare se è presente il nodo "properties" nella QVariantMap. Infatti la presenza del membro properties, anche se vuoto, è obbligatorio in un oggetto Feature GeoJSON.

  - La FeatureCollection è una composizione di oggetti di tipo Feature. Il valore corrispondente alla chiave "data" è una QVariantList popolata da QVariantMaps. 



- **L'exporter:**

  L'exporter accetta in ingresso una QVariantList strutturata come spiegato sopra, e restituisce un QJsonDocument. Come è facile comprendere, nel caso in cui venisse processata attraverso l'exporter la QVariantList prodotta dall'importazione di un dato documento GeoJSON, il medesimo exporter restituirebbe un documento equivalente all'originale dal punto di vista dei dati contenuti, ma non della forma con cui questi vengono esposti.



- **Il metodo "prettyPrint":**

Il metodo prettyPrint consiste in un secondo exporter creato a scopo di debug. Esso restituisce una rappresentazione conforme allo standard JSON, della QVariantList restituita dall'importer.
Questa funzione ci consente di avere una rappresentazione facilmente leggibile della struttura dati costituita da QVariantList e QVariantMap nidificate molteplici volte. Nella rappresentazione JSON ad ogni QVariantList corrisponde una "array" e a ogni QVariantMap un "object". Le geometrie Qt usate dal parser per importare quelle GeoJSON sono rappresentate come JSON objects.



5. #### Visualizzazione di elementi su una mappa in QtLocation

- MapItemView

- Model e Delegate



6. #### Test:

- Funzionamento dell'app di test

- Il delegate

- L'extractor

- Autotest per la classe



7. #### **Pubblicazioni ed esempi:**

- [Google Summer of Code](https://summerofcode.withgoogle.com/archive/2018/projects/5180918117433344)

- [Qt Codereview](https://codereview.qt-project.org/#/)

- [GitHub](https://github.com/jdotsh)
