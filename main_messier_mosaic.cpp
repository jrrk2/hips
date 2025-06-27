// main_messier_mosaic.cpp - Messier object selection version
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QNetworkRequest>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QFile>
#include <QComboBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>
#include <QCheckBox>
#include "ProperHipsClient.h"
#include "MessierCatalog.h"

class MessierMosaicCreator : public QWidget {
    Q_OBJECT

public:
    explicit MessierMosaicCreator(QWidget *parent = nullptr);
    void createMosaic(const MessierObject& messierObj);

private slots:
    void onObjectSelectionChanged();
    void onCreateMosaicClicked();
    void onTileDownloaded();
    void processNextTile();
    void assembleFinalMosaic();

private:
    ProperHipsClient* m_hipsClient;
    QNetworkAccessManager* m_networkManager;
    
    // UI Components
    QComboBox* m_objectSelector;
    QPushButton* m_createButton;
    QLabel* m_objectInfoLabel;
    QTextEdit* m_objectDetails;
    QLabel* m_previewLabel;
    QLabel* m_statusLabel;
    QCheckBox* m_zoomToObjectCheckBox;
    QComboBox* m_gridSizeSelector;
    
    // Current selection and settings
    MessierObject m_currentObject;
    QImage m_fullMosaic;  // Store the full mosaic for zooming
    int m_gridWidth;      // Current grid width (e.g., 3, 4, 6)
    int m_gridHeight;     // Current grid height (e.g., 3, 4, 6)
    
    // Simple tile structure for 3x3 grid
    struct SimpleTile {
        int gridX, gridY;
        long long healpixPixel;
        QString filename;
        QString url;
        QImage image;
        bool downloaded;
    };
    
    QList<SimpleTile> m_tiles;
    int m_currentTileIndex;
    QString m_outputDir;
    QDateTime m_downloadStartTime;
    
    void setupUI();
    void updateObjectInfo();
    void createTileGrid(const SkyPosition& position);
    void downloadTile(int tileIndex);
    void saveProgressReport();
    bool checkExistingTile(const SimpleTile& tile);
    bool isValidJpeg(const QString& filename);
    QImage createZoomedView(const QImage& fullMosaic);
    void updatePreviewDisplay();
    QPoint findBrightnessCenter(const QImage& image);
    QImage applyGaussianBlur(const QImage& image, int radius);
    void updateGridSize();
    QString getGridDisplayName(int width, int height);
    void updateGridRecommendation();
    QString getRecommendedGridSize();
    void testGridGeneration();  // Add generation test method
    void testGridValidation();  // Add validation test method
};

MessierMosaicCreator::MessierMosaicCreator(QWidget *parent) : QWidget(parent) {
    m_hipsClient = new ProperHipsClient(this);
    m_networkManager = new QNetworkAccessManager(this);
    m_currentTileIndex = 0;
    
    // Default grid size
    m_gridWidth = 3;
    m_gridHeight = 3;
    
    // Create output directory
    m_outputDir = "messier_mosaics";
    QDir().mkpath(m_outputDir);
    
    setupUI();
    
    qDebug() << "=== Messier Object Mosaic Creator ===";
    qDebug() << "Select any Messier object and grid size to create HiPS mosaics!";
}

void MessierMosaicCreator::setupUI() {
    setWindowTitle("Messier Object Mosaic Creator");
    setMinimumSize(800, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Object selection group
    QGroupBox* selectionGroup = new QGroupBox("Messier Object Selection", this);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);
    
    QHBoxLayout* selectorLayout = new QHBoxLayout();
    selectorLayout->addWidget(new QLabel("Select Object:", this));
    
    m_objectSelector = new QComboBox(this);
    m_objectSelector->setMinimumWidth(300);
    
    // Populate with Messier objects
    QStringList objectNames = MessierCatalog::getObjectNames();
    for (const QString& name : objectNames) {
        m_objectSelector->addItem(name);
    }
    
    connect(m_objectSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MessierMosaicCreator::onObjectSelectionChanged);
    
    m_createButton = new QPushButton("Create 3x3 Mosaic", this);
    connect(m_createButton, &QPushButton::clicked, this, &MessierMosaicCreator::onCreateMosaicClicked);
    
    m_zoomToObjectCheckBox = new QCheckBox("Zoom to object size", this);
    m_zoomToObjectCheckBox->setToolTip("Crop display to show only the catalogued size of the Messier object");
    connect(m_zoomToObjectCheckBox, &QCheckBox::toggled, this, &MessierMosaicCreator::updatePreviewDisplay);
    
    // Grid size selector
    m_gridSizeSelector = new QComboBox(this);
    m_gridSizeSelector->addItem("3×3 grid (41 arcmin)", QVariantList{3, 3});
    m_gridSizeSelector->addItem("4×4 grid (55 arcmin)", QVariantList{4, 4});
    m_gridSizeSelector->addItem("5×5 grid (69 arcmin)", QVariantList{5, 5});
    m_gridSizeSelector->addItem("6×6 grid (83 arcmin)", QVariantList{6, 6});
    m_gridSizeSelector->addItem("8×8 grid (110 arcmin)", QVariantList{8, 8});
    m_gridSizeSelector->addItem("10×10 grid (137 arcmin)", QVariantList{10, 10});
    m_gridSizeSelector->addItem("12×12 grid (165 arcmin)", QVariantList{12, 12});
    m_gridSizeSelector->addItem("15×15 grid (206 arcmin)", QVariantList{15, 15});
    m_gridSizeSelector->addItem("4×3 grid (55×41 arcmin)", QVariantList{4, 3});
    m_gridSizeSelector->addItem("6×4 grid (83×55 arcmin)", QVariantList{6, 4});
    m_gridSizeSelector->addItem("8×6 grid (110×83 arcmin)", QVariantList{8, 6});
    m_gridSizeSelector->addItem("10×8 grid (137×110 arcmin)", QVariantList{10, 8});
    m_gridSizeSelector->addItem("15×8 grid (206×110 arcmin)", QVariantList{15, 8});
    m_gridSizeSelector->addItem("20×10 grid (275×137 arcmin)", QVariantList{20, 10});
    m_gridSizeSelector->setToolTip("Select mosaic grid size - larger grids show more context");
    connect(m_gridSizeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MessierMosaicCreator::updateGridSize);
    
    selectorLayout->addWidget(m_objectSelector);
    selectorLayout->addWidget(m_gridSizeSelector);
    selectorLayout->addWidget(m_createButton);
    selectorLayout->addWidget(m_zoomToObjectCheckBox);
    selectorLayout->addStretch();
    
    selectionLayout->addLayout(selectorLayout);
    
    // Object info display
    m_objectInfoLabel = new QLabel("Select an object above", this);
    m_objectInfoLabel->setFont(QFont("Arial", 12, QFont::Bold));
    selectionLayout->addWidget(m_objectInfoLabel);
    
    m_objectDetails = new QTextEdit(this);
    m_objectDetails->setMaximumHeight(100);
    m_objectDetails->setReadOnly(true);
    selectionLayout->addWidget(m_objectDetails);
    
    mainLayout->addWidget(selectionGroup);
    
    // Status and preview group
    QGroupBox* resultsGroup = new QGroupBox("Mosaic Results", this);
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    m_statusLabel = new QLabel("Ready to create mosaic", this);
    resultsLayout->addWidget(m_statusLabel);
    
    m_previewLabel = new QLabel(this);
    m_previewLabel->setMinimumSize(400, 400);
    m_previewLabel->setMaximumSize(400, 400);  // Force square aspect ratio
    m_previewLabel->setScaledContents(true);
    m_previewLabel->setStyleSheet("border: 1px solid gray; background-color: black;");
    m_previewLabel->setText("Mosaic preview will appear here");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    
    // Center the preview label in a horizontal layout to maintain square shape
    QHBoxLayout* previewLayout = new QHBoxLayout();
    previewLayout->addStretch();
    previewLayout->addWidget(m_previewLabel);
    previewLayout->addStretch();
    resultsLayout->addLayout(previewLayout);
    
    mainLayout->addWidget(resultsGroup);
    
    // Initialize with first object
    onObjectSelectionChanged();
    
    // Run grid validation test on startup
    testGridValidation();
}

void MessierMosaicCreator::onObjectSelectionChanged() {
    int index = m_objectSelector->currentIndex();
    if (index >= 0) {
        auto objects = MessierCatalog::getAllObjects();
        if (index < objects.size()) {
            m_currentObject = objects[index];
            updateObjectInfo();
        }
    }
}

void MessierMosaicCreator::updateObjectInfo() {
    // Update object info label
    QString infoText = QString("%1").arg(m_currentObject.name);
    if (!m_currentObject.common_name.isEmpty()) {
        infoText += QString(" (%1)").arg(m_currentObject.common_name);
    }
    m_objectInfoLabel->setText(infoText);
    
    // Update detailed information
    QString details = QString(
        "Type: %1\n"
        "Constellation: %2\n"
        "Coordinates: RA %3°, Dec %4°\n"
        "Magnitude: %5\n"
        "Distance: %6 light years\n"
        "Size: %7 × %8 arcminutes\n"
        "Best viewed: %9\n"
        "Previously imaged: %10\n\n"
        "%11"
    ).arg(MessierCatalog::objectTypeToString(m_currentObject.object_type))
     .arg(MessierCatalog::constellationToString(m_currentObject.constellation))
     .arg(m_currentObject.sky_position.ra_deg, 0, 'f', 3)
     .arg(m_currentObject.sky_position.dec_deg, 0, 'f', 3)
     .arg(m_currentObject.magnitude, 0, 'f', 1)
     .arg(QString::number(m_currentObject.distance_kly * 1000, 'f', 0))  // Convert kly to ly
     .arg(m_currentObject.size_arcmin.width(), 0, 'f', 1)
     .arg(m_currentObject.size_arcmin.height(), 0, 'f', 1)
     .arg(m_currentObject.best_viewed)
     .arg(m_currentObject.has_been_imaged ? "Yes" : "No")
     .arg(m_currentObject.description);
    
    m_objectDetails->setText(details);
}

void MessierMosaicCreator::onCreateMosaicClicked() {
    if (m_currentObject.name.isEmpty()) {
        qDebug() << "No object selected";
        return;
    }
    
    qDebug() << QString("\n=== Creating Mosaic for %1 ===").arg(m_currentObject.name);
    
    m_createButton->setEnabled(false);
    m_statusLabel->setText(QString("Creating mosaic for %1...").arg(m_currentObject.name));
    
    createMosaic(m_currentObject);
}

void MessierMosaicCreator::createMosaic(const MessierObject& messierObj) {
    qDebug() << QString("Creating 3x3 mosaic for %1 at coordinates RA=%.3f°, Dec=%.3f°")
                .arg(messierObj.name)
                .arg(messierObj.sky_position.ra_deg)
                .arg(messierObj.sky_position.dec_deg);
    
    createTileGrid(messierObj.sky_position);
    
    qDebug() << QString("Starting download of %1 tiles...").arg(m_tiles.size());
    m_currentTileIndex = 0;
    processNextTile();
}
// Fixed createTileGrid method for MessierMosaicCreator
void MessierMosaicCreator::createTileGrid(const SkyPosition& position) {
    m_tiles.clear();
    int order = 8;
    
    // Calculate center pixel using HEALPix
    long long centerPixel = m_hipsClient->calculateHealPixel(position, order);
    
    qDebug() << QString("Creating %1×%2 tile grid for %3:")
                .arg(m_gridWidth).arg(m_gridHeight).arg(position.name);
    
    // Generate the grid using the improved algorithm
    QList<QList<long long>> pixelGrid = m_hipsClient->createProperNxMGrid(centerPixel, order, m_gridWidth, m_gridHeight);
    
    if (pixelGrid.isEmpty() || pixelGrid.size() != m_gridHeight || pixelGrid[0].size() != m_gridWidth) {
        qDebug() << "❌ Grid generation failed, using fallback";
        
        // Simple fallback: create grid with center pixel and estimated neighbors
        pixelGrid.clear();
        pixelGrid.resize(m_gridHeight);
        
        long long nside = 1LL << order;
        long long pixelSpacing = nside / 32; // Reasonable spacing estimate
        
        int centerX = m_gridWidth / 2;
        int centerY = m_gridHeight / 2;
        
        for (int y = 0; y < m_gridHeight; y++) {
            pixelGrid[y].resize(m_gridWidth);
            for (int x = 0; x < m_gridWidth; x++) {
                int dx = x - centerX;
                int dy = y - centerY;
                pixelGrid[y][x] = centerPixel + dy * pixelSpacing * 8 + dx * pixelSpacing;
            }
        }
    }
    
    // Convert pixel grid to tile objects
    for (int y = 0; y < m_gridHeight; y++) {
        for (int x = 0; x < m_gridWidth; x++) {
            SimpleTile tile;
            tile.gridX = x;
            tile.gridY = y;
            tile.healpixPixel = pixelGrid[y][x];
            tile.downloaded = false;
            
            // Build filename based on HEALPix parameters only (for caching across objects)
            tile.filename = QString("%1/hips_order%2_pixel%3.jpg")
                           .arg(m_outputDir).arg(order).arg(tile.healpixPixel);
            
            int dir = (tile.healpixPixel / 10000) * 10000;
            tile.url = QString("http://alasky.u-strasbg.fr/DSS/DSSColor/Norder%1/Dir%2/Npix%3.jpg")
                      .arg(order).arg(dir).arg(tile.healpixPixel);
            
            if (tile.healpixPixel == centerPixel) {
                qDebug() << QString("  Grid(%1,%2): HEALPix %3 ★ TARGET TILE! ★")
                            .arg(x).arg(y).arg(tile.healpixPixel);
            } else {
                qDebug() << QString("  Grid(%1,%2): HEALPix %3")
                            .arg(x).arg(y).arg(tile.healpixPixel);
            }
            
            m_tiles.append(tile);
        }
    }
    
    qDebug() << QString("Created %1×%2 grid (%3 tiles) for %4")
                .arg(m_gridWidth).arg(m_gridHeight).arg(m_tiles.size()).arg(position.name);
}

// Fixed assembleFinalMosaic to handle variable grid sizes
void MessierMosaicCreator::assembleFinalMosaic() {
    qDebug() << QString("\n=== Assembling %1 Mosaic ===").arg(m_currentObject.name);
    
    int successfulTiles = 0;
    for (const SimpleTile& tile : m_tiles) {
        if (tile.downloaded && !tile.image.isNull()) {
            successfulTiles++;
        }
    }
    
    qDebug() << QString("Downloaded %1/%2 tiles for %3")
                .arg(successfulTiles).arg(m_tiles.size()).arg(m_currentObject.name);
    
    if (successfulTiles == 0) {
        qDebug() << "❌ No tiles downloaded successfully";
        m_statusLabel->setText(QString("Failed to download tiles for %1").arg(m_currentObject.name));
        m_createButton->setEnabled(true);
        return;
    }
    
    // FIXED: Create variable-size mosaic based on actual grid dimensions
    int tileSize = 512;
    int mosaicWidth = m_gridWidth * tileSize;
    int mosaicHeight = m_gridHeight * tileSize;
    
    QImage finalMosaic(mosaicWidth, mosaicHeight, QImage::Format_RGB32);
    finalMosaic.fill(Qt::black);
    
    QPainter painter(&finalMosaic);
    
    int tilesPlaced = 0;
    
    qDebug() << QString("Placing tiles for %1 in %2x%3 grid:")
                .arg(m_currentObject.name).arg(m_gridWidth).arg(m_gridHeight);
    
    for (const SimpleTile& tile : m_tiles) {
        if (!tile.downloaded || tile.image.isNull()) {
            qDebug() << QString("  Skipping tile %1,%2 - not downloaded").arg(tile.gridX).arg(tile.gridY);
            continue;
        }
        
        // Calculate placement position
        int pixelX = tile.gridX * tileSize;
        int pixelY = tile.gridY * tileSize;
        
        // Draw the tile
        painter.drawImage(pixelX, pixelY, tile.image);
        
        tilesPlaced++;
        
        qDebug() << QString("  ✅ Placed tile (%1,%2) at pixel (%3,%4)")
                    .arg(tile.gridX).arg(tile.gridY).arg(pixelX).arg(pixelY);
    }
    
    // Add crosshairs and label at center
    painter.setPen(QPen(Qt::yellow, 3));
    int centerX = mosaicWidth / 2;
    int centerY = mosaicHeight / 2;
    
    // Draw crosshairs
    painter.drawLine(centerX - 30, centerY, centerX + 30, centerY);
    painter.drawLine(centerX, centerY - 30, centerX, centerY + 30);
    
    // Add object label
    painter.setPen(QPen(Qt::yellow, 1));
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    QString labelText = m_currentObject.name;
    if (!m_currentObject.common_name.isEmpty()) {
        labelText = m_currentObject.common_name;
    }
    painter.drawText(centerX + 40, centerY - 10, labelText);
    
    // Add object type label
    painter.setFont(QFont("Arial", 10));
    painter.drawText(centerX + 40, centerY + 10, 
                    MessierCatalog::objectTypeToString(m_currentObject.object_type));
    
    painter.end();
    
    // Store the full mosaic for potential zooming
    m_fullMosaic = finalMosaic;
    
    // Save final mosaic
    QString objectName = m_currentObject.name.toLower();
    QString gridSizeName = QString("%1x%2").arg(m_gridWidth).arg(m_gridHeight);
    QString mosaicFilename = QString("%1/%2_mosaic_%3.png").arg(m_outputDir).arg(objectName).arg(gridSizeName);
    bool saved = finalMosaic.save(mosaicFilename);
    
    qDebug() << QString("\n🖼️  %1 mosaic complete!").arg(m_currentObject.name);
    qDebug() << QString("📁 Size: %1×%2 pixels (%3 tiles placed)")
                .arg(mosaicWidth).arg(mosaicHeight).arg(tilesPlaced);
    qDebug() << QString("📁 Saved to: %1 (%2)")
                .arg(mosaicFilename).arg(saved ? "SUCCESS" : "FAILED");
    
    // FIXED: Scale preview to fit 400x400 while maintaining aspect ratio
    int previewSize = 400;
    QPixmap preview = QPixmap::fromImage(finalMosaic.scaled(previewSize, previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_previewLabel->setPixmap(preview);
    
    // Also save a smaller preview
    QString previewFilename = QString("%1/%2_preview_%3.jpg").arg(m_outputDir).arg(objectName).arg(gridSizeName);
    QImage preview512 = finalMosaic.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    preview512.save(previewFilename);
    qDebug() << QString("📁 Preview: %1").arg(previewFilename);
    
    saveProgressReport();
    
    m_statusLabel->setText(QString("✅ %1 mosaic complete! (%2×%3 grid, %4 tiles)")
                          .arg(m_currentObject.name).arg(m_gridWidth).arg(m_gridHeight).arg(tilesPlaced));
    
    qDebug() << QString("\n🎯 %1 MOSAIC COMPLETE!").arg(m_currentObject.name);
    qDebug() << QString("✅ %1 should be visible in the center tile with crosshairs").arg(labelText);
    
    m_createButton->setEnabled(true);
}

// Add method to test grid generation before creating mosaic
void MessierMosaicCreator::testGridGeneration() {
    if (m_currentObject.name.isEmpty()) {
        qDebug() << "No object selected for grid test";
        return;
    }
    
    qDebug() << QString("\n=== Testing Grid Generation for %1 ===").arg(m_currentObject.name);
    
    int order = 8;
    long long centerPixel = m_hipsClient->calculateHealPixel(m_currentObject.sky_position, order);
    
    qDebug() << QString("Object: %1 at pixel %2 (order %3)")
                .arg(m_currentObject.name).arg(centerPixel).arg(order);
    
    if (centerPixel < 0) {
        qDebug() << "❌ Failed to calculate center pixel";
        return;
    }
    
    // Test the current grid size
    QList<QList<long long>> testGrid = m_hipsClient->createProperNxMGrid(centerPixel, order, m_gridWidth, m_gridHeight);
    
    if (testGrid.isEmpty()) {
        qDebug() << QString("❌ Grid generation failed for %1×%2").arg(m_gridWidth).arg(m_gridHeight);
        return;
    }
    
    if (testGrid.size() != m_gridHeight || testGrid[0].size() != m_gridWidth) {
        qDebug() << QString("❌ Grid dimensions wrong: expected %1×%2, got %3×%4")
                    .arg(m_gridWidth).arg(m_gridHeight)
                    .arg(testGrid[0].size()).arg(testGrid.size());
        return;
    }
    
    // Verify center pixel
    int centerX = m_gridWidth / 2;
    int centerY = m_gridHeight / 2;
    long long actualCenter = testGrid[centerY][centerX];
    
    if (actualCenter == centerPixel) {
        qDebug() << QString("✅ Grid generation successful for %1×%2")
                    .arg(m_gridWidth).arg(m_gridHeight);
        qDebug() << QString("✅ Center pixel verified at (%1,%2): %3")
                    .arg(centerX).arg(centerY).arg(actualCenter);
    } else {
        qDebug() << QString("⚠️  Grid center mismatch: expected %1, got %2")
                    .arg(centerPixel).arg(actualCenter);
    }
    
    // Count valid pixels
    int validPixels = 0;
    for (int y = 0; y < m_gridHeight; y++) {
        for (int x = 0; x < m_gridWidth; x++) {
            if (testGrid[y][x] >= 0) {
                validPixels++;
            }
        }
    }
    
    double coverage = double(validPixels) / double(m_gridWidth * m_gridHeight) * 100.0;
    qDebug() << QString("Coverage: %1/%2 pixels valid (%3%)")
                .arg(validPixels).arg(m_gridWidth * m_gridHeight).arg(coverage, 0, 'f', 1);
    
    if (coverage >= 95.0) {
        qDebug() << QString("✅ Grid ready for mosaic creation");
    } else {
        qDebug() << QString("⚠️  Low pixel coverage - mosaic may have gaps");
    }
}

void MessierMosaicCreator::processNextTile() {
    if (m_currentTileIndex >= m_tiles.size()) {
        assembleFinalMosaic();
        return;
    }
    
    // Check if tile already exists and is valid before downloading
    SimpleTile& tile = m_tiles[m_currentTileIndex];
    if (checkExistingTile(tile)) {
    qDebug() << QString("✓ Using existing tile %1/%2: %3")
                    .arg(m_currentTileIndex + 1).arg(m_tiles.size())
                    .arg(QFileInfo(tile.filename).fileName());
        
        m_statusLabel->setText(QString("Using existing tile %1/%2 for %3...")
                              .arg(m_currentTileIndex + 1).arg(m_tiles.size()).arg(m_currentObject.name));
        
        m_currentTileIndex++;
        
        // Small delay before processing next tile
        QTimer::singleShot(100, this, &MessierMosaicCreator::processNextTile);
        return;
    }
    
    downloadTile(m_currentTileIndex);
}

void MessierMosaicCreator::downloadTile(int tileIndex) {
    if (tileIndex >= m_tiles.size()) return;
    
    const SimpleTile& tile = m_tiles[tileIndex];
    
    qDebug() << QString("Downloading tile %1/%2: Grid(%3,%4) HEALPix %5")
                .arg(tileIndex + 1).arg(m_tiles.size())
                .arg(tile.gridX).arg(tile.gridY)
                .arg(tile.healpixPixel);
    
    m_statusLabel->setText(QString("Downloading tile %1/%2 for %3...")
                          .arg(tileIndex + 1).arg(m_tiles.size()).arg(m_currentObject.name));
    
    // Create network request
    QNetworkRequest request(QUrl(tile.url));
    request.setHeader(QNetworkRequest::UserAgentHeader, "MessierMosaicCreator/1.0");
    request.setRawHeader("Accept", "image/*");
    
    m_downloadStartTime = QDateTime::currentDateTime();
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Store tile index in reply
    reply->setProperty("tileIndex", tileIndex);
    
    connect(reply, &QNetworkReply::finished, this, &MessierMosaicCreator::onTileDownloaded);
    
    // Set timeout
    QTimer::singleShot(15000, reply, &QNetworkReply::abort);
}

void MessierMosaicCreator::onTileDownloaded() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    int tileIndex = reply->property("tileIndex").toInt();
    if (tileIndex >= m_tiles.size()) {
        reply->deleteLater();
        return;
    }
    
    SimpleTile& tile = m_tiles[tileIndex];
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        tile.image.loadFromData(imageData);
        
        if (!tile.image.isNull()) {
            bool saved = tile.image.save(tile.filename);
            tile.downloaded = true;
            
            qint64 downloadTime = m_downloadStartTime.msecsTo(QDateTime::currentDateTime());
            
    qDebug() << QString("✅ Tile %1/%2 downloaded: %3ms, %4 bytes, %5x%6 pixels%7")
                        .arg(tileIndex + 1).arg(m_tiles.size())
                        .arg(downloadTime).arg(imageData.size())
                        .arg(tile.image.width()).arg(tile.image.height())
                        .arg(saved ? ", cached" : ", cache failed");
                        
            qDebug() << QString("  → Cached as: %1")
                        .arg(QFileInfo(tile.filename).fileName());
        } else {
            qDebug() << QString("❌ Tile %1/%2 - invalid image data")
                        .arg(tileIndex + 1).arg(m_tiles.size());
        }
    } else {
        qDebug() << QString("❌ Tile %1/%2 download failed: %3")
                    .arg(tileIndex + 1).arg(m_tiles.size())
                    .arg(reply->errorString());
    }
    
    reply->deleteLater();
    
    m_currentTileIndex++;
    
    // Small delay between downloads
    QTimer::singleShot(500, this, &MessierMosaicCreator::processNextTile);
}

void MessierMosaicCreator::saveProgressReport() {
    QString objectName = m_currentObject.name.toLower();
    QString reportFile = QString("%1/%2_mosaic_report.txt").arg(m_outputDir).arg(objectName);
    QFile file(reportFile);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Could not save progress report";
        return;
    }
    
    QTextStream out(&file);
    out << QString("%1 Mosaic Report\n").arg(m_currentObject.name);
    out << "Generated: " << QDateTime::currentDateTime().toString() << "\n\n";
    
    out << QString("Object: %1").arg(m_currentObject.name);
    if (!m_currentObject.common_name.isEmpty()) {
        out << QString(" (%1)").arg(m_currentObject.common_name);
    }
    out << "\n";
    out << QString("Type: %1\n").arg(MessierCatalog::objectTypeToString(m_currentObject.object_type));
    out << QString("Coordinates: RA %1°, Dec %2°\n")
           .arg(m_currentObject.sky_position.ra_deg, 0, 'f', 3)
           .arg(m_currentObject.sky_position.dec_deg, 0, 'f', 3);
    out << QString("Magnitude: %1\n").arg(m_currentObject.magnitude, 0, 'f', 1);
    out << QString("Distance: %1 light years\n").arg(QString::number(m_currentObject.distance_kly * 1000, 'f', 0));
    out << QString("Best viewed: %1\n\n").arg(m_currentObject.best_viewed);
    
    out << "3x3 Grid Layout:\n";
    out << "Grid_X,Grid_Y,HEALPix_Pixel,Downloaded,ImageSize,Filename\n";
    
    for (const SimpleTile& tile : m_tiles) {
        out << QString("%1,%2,%3,%4,%5x%6,%7\n")
               .arg(tile.gridX).arg(tile.gridY)
               .arg(tile.healpixPixel)
               .arg(tile.downloaded ? "YES" : "NO")
               .arg(tile.image.width()).arg(tile.image.height())
               .arg(tile.filename);
    }
    
    file.close();
    qDebug() << "Report saved:" << reportFile;
}

bool MessierMosaicCreator::checkExistingTile(const SimpleTile& tile) {
    // Check if file exists
    QFileInfo fileInfo(tile.filename);
    if (!fileInfo.exists()) {
        return false;
    }
    
    // Check if file size is reasonable (not empty or too small)
    if (fileInfo.size() < 1024) {  // Less than 1KB suggests corrupted file
        qDebug() << QString("Existing tile %1 is too small (%2 bytes), will re-download")
                    .arg(fileInfo.fileName()).arg(fileInfo.size());
        return false;
    }
    
    // Check if it's a valid JPEG
    if (!isValidJpeg(tile.filename)) {
        qDebug() << QString("Existing tile %1 is not a valid JPEG, will re-download")
                    .arg(fileInfo.fileName());
        return false;
    }
    
    // Load the image to verify it's valid and update the tile structure
    SimpleTile* mutableTile = const_cast<SimpleTile*>(&tile);
    mutableTile->image.load(tile.filename);
    
    if (mutableTile->image.isNull()) {
        qDebug() << QString("Existing tile %1 failed to load as image, will re-download")
                    .arg(fileInfo.fileName());
        return false;
    }
    
    // Mark as downloaded since we have a valid existing file
    mutableTile->downloaded = true;
    
    qDebug() << QString("Found valid existing tile: %1 (%2 bytes, %3x%4 pixels)")
                .arg(fileInfo.fileName())
                .arg(fileInfo.size())
                .arg(mutableTile->image.width())
                .arg(mutableTile->image.height());
    
    return true;
}

bool MessierMosaicCreator::isValidJpeg(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Read first few bytes to check JPEG magic number
    QByteArray header = file.read(4);
    file.close();
    
    // JPEG files start with FF D8 FF
    if (header.size() >= 3 && 
        static_cast<unsigned char>(header[0]) == 0xFF && 
        static_cast<unsigned char>(header[1]) == 0xD8 && 
        static_cast<unsigned char>(header[2]) == 0xFF) {
        return true;
    }
    
    return false;
}

void MessierMosaicCreator::updateGridSize() {
    QVariantList gridData = m_gridSizeSelector->currentData().toList();
    if (gridData.size() == 2) {
        m_gridWidth = gridData[0].toInt();
        m_gridHeight = gridData[1].toInt();
        
        qDebug() << QString("Grid size changed to %1×%2").arg(m_gridWidth).arg(m_gridHeight);
        
        // Update the create button text
        m_createButton->setText(QString("Create %1×%2 Mosaic").arg(m_gridWidth).arg(m_gridHeight));
        
        // Clear any existing mosaic since grid size changed
        m_fullMosaic = QImage();
        m_previewLabel->clear();
        m_previewLabel->setText("Select grid size and create mosaic");
    }
}

QString MessierMosaicCreator::getGridDisplayName(int width, int height) {
    if (width == height) {
        return QString("%1×%1").arg(width);
    } else {
        return QString("%1×%2").arg(width).arg(height);
    }
}

QString MessierMosaicCreator::getRecommendedGridSize() {
    // Calculate the larger dimension of the object
    double maxObjectSize = std::max(m_currentObject.size_arcmin.width(), 
                                   m_currentObject.size_arcmin.height());
    
    // Base field size for 3×3 grid is 41.2 arcmin
    const double BASE_FIELD = 41.2;
    
    // Object should occupy roughly 50-80% of the field for good framing
    double targetField = maxObjectSize / 0.65;  // Object takes ~65% of field
    
    // Calculate required grid size
    double gridScale = targetField / BASE_FIELD;
    int recommendedSize = std::max(3, (int)ceil(gridScale));
    
    // For very elongated objects, suggest rectangular grids
    double aspectRatio = m_currentObject.size_arcmin.width() / m_currentObject.size_arcmin.height();
    
    QString recommendation;
    
    if (aspectRatio > 2.0 || aspectRatio < 0.5) {
        // Highly elongated object - suggest rectangular grid
        int width = recommendedSize;
        int height = recommendedSize;
        
        if (aspectRatio > 2.0) {
            // Wide object - make grid wider
            width = (int)(recommendedSize * 1.5);
        } else {
            // Tall object - make grid taller  
            height = (int)(recommendedSize * 1.5);
        }
        
        recommendation = QString("%1×%2 grid (%3×%4 arcmin)")
                         .arg(width).arg(height)
                         .arg((width * BASE_FIELD / 3.0), 0, 'f', 0)
                         .arg((height * BASE_FIELD / 3.0), 0, 'f', 0);
    } else {
        // Regular object - suggest square grid
        recommendation = QString("%1×%1 grid (%2 arcmin)")
                        .arg(recommendedSize)
                        .arg((recommendedSize * BASE_FIELD / 3.0), 0, 'f', 0);
    }
    
    // Add size category explanation
    if (maxObjectSize < 5.0) {
        recommendation += " (small object)";
    } else if (maxObjectSize < 20.0) {
        recommendation += " (medium object)";
    } else if (maxObjectSize < 60.0) {
        recommendation += " (large object)";
    } else {
        recommendation += " (very large object)";
    }
    
    return recommendation;
}

void MessierMosaicCreator::updateGridRecommendation() {
    // Get recommended grid parameters
    double maxObjectSize = std::max(m_currentObject.size_arcmin.width(), 
                                   m_currentObject.size_arcmin.height());
    const double BASE_FIELD = 41.2;
    double targetField = maxObjectSize / 0.65;
    double gridScale = targetField / BASE_FIELD;
    int recommendedSize = std::max(3, (int)ceil(gridScale));
    
    // Update tooltip with specific guidance
    QString tooltip = QString(
        "Object size: %1×%2 arcmin\n"
        "Recommended: %3×%3 grid or larger\n"
        "Current selection covers %4 arcmin\n\n"
        "• 3×3 (41 arcmin) - Small objects (<10 arcmin)\n"
        "• 6×6 (83 arcmin) - Medium objects (10-40 arcmin)\n"
        "• 10×10 (137 arcmin) - Large objects (40-80 arcmin)\n"
        "• 15×15 (206 arcmin) - Very large objects (>80 arcmin)\n"
        "• 20×10 (275×137 arcmin) - M31 Andromeda Galaxy"
    ).arg(m_currentObject.size_arcmin.width(), 0, 'f', 1)
     .arg(m_currentObject.size_arcmin.height(), 0, 'f', 1)
     .arg(recommendedSize)
     .arg((std::max(m_gridWidth, m_gridHeight) * BASE_FIELD / 3.0), 0, 'f', 0);
    
    m_gridSizeSelector->setToolTip(tooltip);
}

void MessierMosaicCreator::testGridValidation() {
    qDebug() << "\n=== GRID VALIDATION TEST ===";
    
    // Test with M51 coordinates
    SkyPosition testPos = {202.4695833, 47.1951667, "M51_Test", "Grid validation test"};
    long long testPixel = m_hipsClient->calculateHealPixel(testPos, 8);
    
    qDebug() << QString("Test center pixel: %1").arg(testPixel);
    
    // Generate reference 3×3 grid
    QList<QList<long long>> reference3x3 = m_hipsClient->createProper3x3Grid(testPixel, 8);
    
    qDebug() << "\nReference 3×3 grid:";
    for (int y = 0; y < 3; y++) {
        QString row = "  ";
        for (int x = 0; x < 3; x++) {
            row += QString("[%1] ").arg(reference3x3[y][x]);
        }
        qDebug() << row;
    }
    
    // Test different grid sizes
    QList<QPair<int,int>> testSizes = {{4,4}, {5,5}, {6,6}, {4,3}, {6,4}};
    
    for (const auto& size : testSizes) {
        int width = size.first;
        int height = size.second;
        
        qDebug() << QString("\n--- Testing %1×%2 grid ---").arg(width).arg(height);
        
        // Generate larger grid
        QList<QList<long long>> largerGrid = m_hipsClient->createProperNxMGrid(testPixel, 8, width, height);
        
        // Extract center 3×3 from larger grid
        int centerX = width / 2;
        int centerY = height / 2;
        
        bool validationPassed = true;
        QList<QList<long long>> extracted3x3;
        extracted3x3.resize(3);
        
        qDebug() << QString("Extracting center 3×3 from position (%1,%2):").arg(centerX).arg(centerY);
        
        for (int y = 0; y < 3; y++) {
            extracted3x3[y].resize(3);
            QString row = "  ";
            for (int x = 0; x < 3; x++) {
                int sourceY = centerY - 1 + y;  // -1,0,1 offset from center
                int sourceX = centerX - 1 + x;  // -1,0,1 offset from center
                
                if (sourceX >= 0 && sourceX < width && sourceY >= 0 && sourceY < height) {
                    extracted3x3[y][x] = largerGrid[sourceY][sourceX];
                    row += QString("[%1] ").arg(extracted3x3[y][x]);
                    
                    // Compare with reference
                    if (extracted3x3[y][x] != reference3x3[y][x]) {
                        qDebug() << QString("❌ MISMATCH at (%1,%2): extracted=%3, reference=%4")
                                    .arg(x).arg(y).arg(extracted3x3[y][x]).arg(reference3x3[y][x]);
                        validationPassed = false;
                    }
                } else {
                    row += "[OOB] ";
                    qDebug() << QString("❌ OUT OF BOUNDS at (%1,%2)").arg(x).arg(y);
                    validationPassed = false;
                }
            }
            qDebug() << row;
        }
        
        if (validationPassed) {
            qDebug() << QString("✅ %1×%2 grid validation PASSED").arg(width).arg(height);
        } else {
            qDebug() << QString("❌ %1×%2 grid validation FAILED").arg(width).arg(height);
        }
    }
    
    qDebug() << "\n=== END GRID VALIDATION TEST ===\n";
}

void MessierMosaicCreator::updatePreviewDisplay() {
    if (m_fullMosaic.isNull()) {
        return;  // No mosaic to display yet
    }
    
    QImage displayImage;
    
    if (m_zoomToObjectCheckBox->isChecked()) {
        displayImage = createZoomedView(m_fullMosaic);
        qDebug() << QString("Displaying zoomed view of %1 (%2 × %3 arcmin)")
                    .arg(m_currentObject.name)
                    .arg(m_currentObject.size_arcmin.width(), 0, 'f', 1)
                    .arg(m_currentObject.size_arcmin.height(), 0, 'f', 1);
    } else {
        displayImage = m_fullMosaic;
        qDebug() << QString("Displaying full 3x3 mosaic of %1").arg(m_currentObject.name);
    }
    
    // Scale to fit 400x400 preview while maintaining aspect ratio
    QPixmap preview = QPixmap::fromImage(displayImage.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_previewLabel->setPixmap(preview);
}

QImage MessierMosaicCreator::createZoomedView(const QImage& fullMosaic) {
    if (fullMosaic.isNull()) {
        return QImage();
    }
    
    // First, find the actual center of the object based on brightness
    QPoint actualCenter = findBrightnessCenter(fullMosaic);
    
    qDebug() << QString("Auto-centering %1: geometric center (%2,%3) vs brightness center (%4,%5)")
                .arg(m_currentObject.name)
                .arg(fullMosaic.width()/2).arg(fullMosaic.height()/2)
                .arg(actualCenter.x()).arg(actualCenter.y());
    
    // Use REAL plate solve data from M1 mosaic, scaled for different grid sizes:
    // 3x3 grid: 41.2 x 41.2 arcmin (measured)
    // Larger grids scale proportionally: NxM grid = (N/3 * 41.2) x (M/3 * 41.2) arcmin
    const double ARCSEC_PER_PIXEL = 1.61;  // From actual plate solve
    const double BASE_FIELD_ARCMIN = 41.2;  // 3x3 measured field
    
    // Calculate actual field for current grid size
    double fieldWidth = (m_gridWidth / 3.0) * BASE_FIELD_ARCMIN;
    double fieldHeight = (m_gridHeight / 3.0) * BASE_FIELD_ARCMIN;
    
    // Get object size in arcminutes with adaptive margins
    double objectWidth = m_currentObject.size_arcmin.width();
    double objectHeight = m_currentObject.size_arcmin.height();
    
    // Use more conservative adaptive padding based on object size
    double paddingFactor;
    if (objectWidth < 1.0 || objectHeight < 1.0) {
        paddingFactor = 8.0;  // Extremely small objects get 8x margin
    } else if (objectWidth < 3.0 || objectHeight < 3.0) {
        paddingFactor = 5.0;  // Very small objects get 5x margin  
    } else if (objectWidth < 8.0 || objectHeight < 8.0) {
        paddingFactor = 3.0;  // Small objects get 3x margin
    } else if (objectWidth < 20.0 || objectHeight < 20.0) {
        paddingFactor = 2.0;  // Medium objects get 2x margin
    } else {
        paddingFactor = 1.5;  // Large objects get 1.5x margin
    }
    
    objectWidth *= paddingFactor;
    objectHeight *= paddingFactor;
    
    // Calculate what fraction of the full mosaic the object occupies
    double widthFraction = objectWidth / fieldWidth;
    double heightFraction = objectHeight / fieldHeight;
    
    // ASPECT RATIO PRESERVATION: Use the larger fraction to maintain object proportions
    double zoomFraction = std::max(widthFraction, heightFraction);
    
    // Apply conservative zoom limits
    zoomFraction = std::max(0.3, std::min(1.0, zoomFraction));   // At least 30% of mosaic
    
    // For very small objects, use even more conservative minimum zoom
    if (m_currentObject.size_arcmin.width() < 2.0 || m_currentObject.size_arcmin.height() < 2.0) {
        zoomFraction = std::max(zoomFraction, 0.5);   // Minimum 50% for tiny objects
        qDebug() << QString("  Applied conservative minimum zoom for very small object");
    }
    
    // Calculate crop rectangle with preserved aspect ratio (square crop)
    // The crop will be square, but large enough to contain the entire object with its natural proportions
    int cropSize = static_cast<int>(std::min(fullMosaic.width(), fullMosaic.height()) * zoomFraction);
    int cropWidth = cropSize;
    int cropHeight = cropSize;
    
    int cropX = actualCenter.x() - cropWidth / 2;
    int cropY = actualCenter.y() - cropHeight / 2;
    
    // Ensure crop rectangle is within bounds
    cropX = std::max(0, std::min(cropX, fullMosaic.width() - cropWidth));
    cropY = std::max(0, std::min(cropY, fullMosaic.height() - cropHeight));
    
    QRect cropRect(cropX, cropY, cropWidth, cropHeight);
    
    // Calculate the actual angular size of the cropped view
    double cropFieldSize = (cropSize * ARCSEC_PER_PIXEL) / 60.0;   // Convert to arcmin
    
    // Calculate offset from geometric center for debugging
    int offsetX = actualCenter.x() - fullMosaic.width()/2;
    int offsetY = actualCenter.y() - fullMosaic.height()/2;
    
    // Calculate object coverage within the crop
    double objectCoverageWidth = (m_currentObject.size_arcmin.width() * paddingFactor) / cropFieldSize * 100.0;
    double objectCoverageHeight = (m_currentObject.size_arcmin.height() * paddingFactor) / cropFieldSize * 100.0;
    
    qDebug() << QString("Zoom calculation for %1 (using plate solve data):").arg(m_currentObject.name);
    qDebug() << QString("  Object size: %1 × %2 arcmin (with %3x padding: %4 × %5)")
                .arg(m_currentObject.size_arcmin.width(), 0, 'f', 1)
                .arg(m_currentObject.size_arcmin.height(), 0, 'f', 1)
                .arg(paddingFactor, 0, 'f', 1)
                .arg(objectWidth, 0, 'f', 1)
                .arg(objectHeight, 0, 'f', 1);
    qDebug() << QString("  Full field: %1 × %2 arcmin (%3 arcsec/pixel)")
                .arg(fieldWidth, 0, 'f', 1)
                .arg(fieldHeight, 0, 'f', 1)
                .arg(ARCSEC_PER_PIXEL, 0, 'f', 2);
    qDebug() << QString("  Crop field: %1 × %2 arcmin (%3×%4 pixels) - SQUARE CROP")
                .arg(cropFieldSize, 0, 'f', 1)
                .arg(cropFieldSize, 0, 'f', 1)
                .arg(cropWidth).arg(cropHeight);
    qDebug() << QString("  Object coverage: %1% × %2% of crop area")
                .arg(objectCoverageWidth, 0, 'f', 1)
                .arg(objectCoverageHeight, 0, 'f', 1);
    qDebug() << QString("  Brightness offset: %1,%2 pixels from geometric center")
                .arg(offsetX).arg(offsetY);
    qDebug() << QString("  Zoom fraction: %1, Crop rect: %2,%3")
                .arg(zoomFraction, 0, 'f', 3)
                .arg(cropX).arg(cropY);
    
    return fullMosaic.copy(cropRect);
}

QPoint MessierMosaicCreator::findBrightnessCenter(const QImage& image) {
    if (image.isNull()) {
        return QPoint(image.width()/2, image.height()/2);
    }
    
    // Convert to grayscale and apply slight blur to reduce noise
    QImage workingImage = image.convertToFormat(QImage::Format_RGB32);
    QImage blurred = applyGaussianBlur(workingImage, 3);
    
    // Create brightness map
    int width = blurred.width();
    int height = blurred.height();
    
    // Find the brightest regions using a weighted centroid approach
    double totalWeightedX = 0.0;
    double totalWeightedY = 0.0;
    double totalWeight = 0.0;
    
    // First pass: find the maximum brightness to normalize
    int maxBrightness = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = blurred.pixel(x, y);
            int brightness = qGray(pixel);
            maxBrightness = std::max(maxBrightness, brightness);
        }
    }
    
    if (maxBrightness == 0) {
        return QPoint(width/2, height/2);  // Fallback to center
    }
    
    // Second pass: calculate weighted centroid using only bright pixels
    // Use threshold to focus on the brightest regions (top 30% of brightness)
    int brightnessThreshold = static_cast<int>(maxBrightness * 0.7);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb pixel = blurred.pixel(x, y);
            int brightness = qGray(pixel);
            
            // Only consider bright pixels for centroid calculation
            if (brightness > brightnessThreshold) {
                // Weight by brightness squared to emphasize the brightest areas
                double weight = static_cast<double>(brightness * brightness);
                
                totalWeightedX += x * weight;
                totalWeightedY += y * weight;
                totalWeight += weight;
            }
        }
    }
    
    QPoint center;
    if (totalWeight > 0) {
        center.setX(static_cast<int>(totalWeightedX / totalWeight));
        center.setY(static_cast<int>(totalWeightedY / totalWeight));
    } else {
        // Fallback to geometric center if no bright regions found
        center = QPoint(width/2, height/2);
    }
    
    // Ensure the center is within the image bounds
    center.setX(std::max(0, std::min(center.x(), width - 1)));
    center.setY(std::max(0, std::min(center.y(), height - 1)));
    
    qDebug() << QString("Brightness analysis: max=%1, threshold=%2, weight=%3, center=(%4,%5)")
                .arg(maxBrightness).arg(brightnessThreshold).arg(totalWeight, 0, 'f', 0)
                .arg(center.x()).arg(center.y());
    
    return center;
}

QImage MessierMosaicCreator::applyGaussianBlur(const QImage& image, int radius) {
    if (image.isNull() || radius <= 0) {
        return image;
    }
    
    QImage result = image.copy();
    int width = result.width();
    int height = result.height();
    
    // Simple box blur approximation of Gaussian blur
    // Apply horizontal blur
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int totalR = 0, totalG = 0, totalB = 0;
            int count = 0;
            
            for (int dx = -radius; dx <= radius; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < width) {
                    QRgb pixel = image.pixel(nx, y);
                    totalR += qRed(pixel);
                    totalG += qGreen(pixel);
                    totalB += qBlue(pixel);
                    count++;
                }
            }
            
            if (count > 0) {
                result.setPixel(x, y, qRgb(totalR/count, totalG/count, totalB/count));
            }
        }
    }
    
    // Apply vertical blur
    QImage final = result.copy();
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int totalR = 0, totalG = 0, totalB = 0;
            int count = 0;
            
            for (int dy = -radius; dy <= radius; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < height) {
                    QRgb pixel = result.pixel(x, ny);
                    totalR += qRed(pixel);
                    totalG += qGreen(pixel);
                    totalB += qBlue(pixel);
                    count++;
                }
            }
            
            if (count > 0) {
                final.setPixel(x, y, qRgb(totalR/count, totalG/count, totalB/count));
            }
        }
    }
    
    return final;
}

// Add missing constellation to string function
QString MessierCatalog::constellationToString(Constellation constellation) {
    switch(constellation) {
        case Constellation::ANDROMEDA: return "Andromeda";
        case Constellation::AQUARIUS: return "Aquarius";
        case Constellation::AURIGA: return "Auriga";
        case Constellation::CANCER: return "Cancer";
        case Constellation::CANES_VENATICI: return "Canes Venatici";
        case Constellation::CANIS_MAJOR: return "Canis Major";
        case Constellation::CAPRICORNUS: return "Capricornus";
        case Constellation::CASSIOPEIA: return "Cassiopeia";
        case Constellation::CETUS: return "Cetus";
        case Constellation::COMA_BERENICES: return "Coma Berenices";
        case Constellation::CYGNUS: return "Cygnus";
        case Constellation::DRACO: return "Draco";
        case Constellation::GEMINI: return "Gemini";
        case Constellation::HERCULES: return "Hercules";
        case Constellation::HYDRA: return "Hydra";
        case Constellation::LEO: return "Leo";
        case Constellation::LEPUS: return "Lepus";
        case Constellation::LYRA: return "Lyra";
        case Constellation::MONOCEROS: return "Monoceros";
        case Constellation::OPHIUCHUS: return "Ophiuchus";
        case Constellation::ORION: return "Orion";
        case Constellation::PEGASUS: return "Pegasus";
        case Constellation::PERSEUS: return "Perseus";
        case Constellation::PISCES: return "Pisces";
        case Constellation::PUPPIS: return "Puppis";
        case Constellation::SAGITTA: return "Sagitta";
        case Constellation::SAGITTARIUS: return "Sagittarius";
        case Constellation::SCORPIUS: return "Scorpius";
        case Constellation::SCUTUM: return "Scutum";
        case Constellation::SERPENS: return "Serpens";
        case Constellation::TAURUS: return "Taurus";
        case Constellation::TRIANGULUM: return "Triangulum";
        case Constellation::URSA_MAJOR: return "Ursa Major";
        case Constellation::VIRGO: return "Virgo";
        case Constellation::VULPECULA: return "Vulpecula";
        default: return "Unknown";
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    qDebug() << "=== Messier Object Mosaic Creator ===";
    qDebug() << "Select any Messier object to create a 3x3 HiPS mosaic!";
    qDebug() << "Available objects from catalog with accurate coordinates\n";
    
    MessierMosaicCreator creator;
    creator.show();
    
    return app.exec();
}

#include "main_messier_mosaic.moc"
