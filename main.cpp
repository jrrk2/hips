// main.cpp - Test program using real HEALPix library
#include <QApplication>
#include <QDebug>
#include "ProperHipsClient.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    qDebug() << "=== Proper HiPS Client with Real HEALPix ===";
    qDebug() << "This will test HiPS surveys using accurate pixel calculations";
    qDebug() << "";
    
    ProperHipsClient client;

    // Connect completion signal to app quit
    QObject::connect(&client, &ProperHipsClient::testingComplete,
                     &app, &QApplication::quit);
    
    // First, test pixel calculation to see the difference
    client.testPixelCalculation();
    
    qDebug() << "\nStarting comprehensive survey testing...";
    qDebug() << "This should fix the 404 errors from the previous test!";
    qDebug() << "";
    
    // Run the full test suite
    client.testAllSurveys();
    
    return app.exec();
}
