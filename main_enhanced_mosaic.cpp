// main_minimal_enhanced_mosaic.cpp - Minimal changes to working version
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
#include <QLineEdit>
#include <QTabWidget>
#include <QFormLayout>
#include <QRegularExpression>
#include <QMessageBox>
#include "ProperHipsClient.h"
#include "MessierCatalog.h"

// Simple coordinate parser (minimal version)
struct SimpleCoordinateParser {
    static SkyPosition parseCoordinates(const QString& raText, const QString& decText, 
                                      const QString& name = "Custom Target") {
        SkyPosition pos;
        pos.name = name;
        pos.description = "User-defined coordinates";
        pos.ra_deg = parseRA(raText);
        pos.dec_deg = parseDec(decText);
        return pos;
    }
    
private:
    static double parseRA(const QString& text) {
        QString clean = text.trimmed();
        
        // Check if it contains colons (sexagesimal format)
        if (clean.contains(':')) {
            QStringList parts = clean.split(':');
            if (parts.size() >= 2) {
                double hours = parts[0].toDouble();
                double minutes = parts[1].toDouble();
                double seconds = parts.size() > 2 ? parts[2].toDouble() : 0.0;
                return (hours + minutes/60.0 + seconds/3600.0) * 15.0; // Convert to degrees
            }
        }
        
        // Check if it contains 'h' (astronomical format)
        if (clean.contains('h')) {
            QRegularExpression re("(\\d+(?:\\.\\d+)?)h(?:(\\d+(?:\\.\\d+)?)m)?(?:(\\d+(?:\\.\\d+)?)s)?");
            QRegularExpressionMatch match = re.match(clean);
            if (match.hasMatch()) {
                double hours = match.captured(1).toDouble();
                double minutes = match.captured(2).isEmpty() ? 0.0 : match.captured(2).toDouble();
                double seconds = match.captured(3).isEmpty() ? 0.0 : match.captured(3).toDouble();
                return (hours + minutes/60.0 + seconds/3600.0) * 15.0;
            }
        }
        
        // Otherwise assume decimal degrees
        double degrees = clean.toDouble();
        // If the value is less than 24, assume it's hours and convert to degrees
        if (degrees <= 24.0) {
            return degrees * 15.0;
        }
        return degrees;
    }
    
    static double parseDec(const QString& text) {
        QString clean = text.trimmed();
        bool negative = clean.startsWith('-');
        if (negative) clean = clean.mid(1);
        if (clean.startsWith('+')) clean = clean.mid(1);
        
        // Check if it contains colons (sexagesimal format)
        if (clean.contains(':')) {
            QStringList parts = clean.split(':');
            if (parts.size() >= 2) {
                double degrees = parts[0].toDouble();
                double minutes = parts[1].toDouble();
                double seconds = parts.size() > 2 ? parts[2].toDouble() : 0.0;
                double result = degrees + minutes/60.0 + seconds/3600.0;
                return negative ? -result : result;
            }
        }
        
        // Check if it contains 'd' (astronomical format)
        if (clean.contains('d')) {
            QRegularExpression re("(\\d+(?:\\.\\d+)?)d(?:(\\d+(?:\\.\\d+)?)m)?(?:(\\d+(?:\\.\\d+)?)s)?");
            QRegularExpressionMatch match = re.match(clean);
            if (match.hasMatch()) {
                double degrees = match.captured(1).toDouble();
                double minutes = match.captured(2).isEmpty() ? 0.0 : match.captured(2).toDouble();
                double seconds = match.captured(3).isEmpty() ? 0.0 : match.captured(3).toDouble();
                double result = degrees + minutes/60.0 + seconds/3600.0;
                return negative ? -result : result;
            }
        }
        
        // Otherwise assume decimal degrees
        double result = clean.toDouble();
        return negative ? -result : result;
    }
};

class MinimalEnhancedMosaicCreator : public QWidget {
    Q_OBJECT

public:
    explicit MinimalEnhancedMosaicCreator(QWidget *parent = nullptr);

private slots:
    void onObjectSelectionChanged();
    void onCreateMosaicClicked();
    void onCreateCustomMosaicClicked();
    void onCoordinatesChanged();
    void onTileDownloaded();
    void processNextTile();
    void assembleFinalMosaic();
    void onTabChanged(int index);

private:
    ProperHipsClient* m_hipsClient;
    QNetworkAccessManager* m_networkManager;
    
    // UI Components
    QTabWidget* m_tabWidget;
    
    // Messier tab (unchanged from original)
    QComboBox* m_objectSelector;
    QPushButton* m_createButton;
    QLabel* m_objectInfoLabel;
    QTextEdit* m_objectDetails;
    
    // Custom coordinates tab (NEW)
    QLineEdit* m_raInput;
    QLineEdit* m_decInput;
    QLineEdit* m_nameInput;
    QPushButton* m_createCustomButton;
    QLabel* m_coordinatePreview;
    
    // Common elements
    QLabel* m_previewLabel;
    QLabel* m_statusLabel;
    QCheckBox* m_zoomToObjectCheckBox;
    
    // Target tracking
    MessierObject m_currentObject;
    SkyPosition m_customTarget;
    bool m_usingCustomCoordinates;
    QImage m_fullMosaic;
    
    // UNCHANGED: Keep all the working tile logic exactly as it was
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
    
    // UI setup
    void setupUI();
    void setupMessierTab();
    void setupCustomTab();
    void updateObjectInfo();
    void updateCoordinatePreview();
    
    // UNCHANGED: Keep all working core algorithms exactly as they were
    void createMosaic(const MessierObject& messierObj);
    void createCustomMosaic(const SkyPosition& target);
    void createTileGrid(const SkyPosition& position);
    void downloadTile(int tileIndex);
    void saveProgressReport(const QString& targetName);
    bool checkExistingTile(const SimpleTile& tile);
    bool isValidJpeg(const QString& filename);
    void updatePreviewDisplay();
    QImage createZoomedView(const QImage& fullMosaic);
    QPoint findBrightnessCenter(const QImage& image);
    QImage applyGaussianBlur(const QImage& image, int radius);
};

MinimalEnhancedMosaicCreator::MinimalEnhancedMosaicCreator(QWidget *parent) 
    : QWidget(parent), m_usingCustomCoordinates(false) {
    
    m_hipsClient = new ProperHipsClient(this);
    m_networkManager = new QNetworkAccessManager(this);
    m_currentTileIndex = 0;
    
    // Create output directory
    m_outputDir = "enhanced_mosaics";
    QDir().mkpath(m_outputDir);
    
    setupUI();
    
    qDebug() << "=== Minimal Enhanced Mosaic Creator ===";
    qDebug() << "Messier objects + custom coordinates with unchanged core algorithms!";
}

void MinimalEnhancedMosaicCreator::setupUI() {
    setWindowTitle("Enhanced Mosaic Creator - Minimal Changes");
    setMinimumSize(800, 700);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    setupMessierTab();
    setupCustomTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Common controls at bottom
    QGroupBox* resultsGroup = new QGroupBox("Results", this);
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    m_zoomToObjectCheckBox = new QCheckBox("Auto-zoom to object size", this);
    connect(m_zoomToObjectCheckBox, &QCheckBox::toggled, this, &MinimalEnhancedMosaicCreator::updatePreviewDisplay);
    resultsLayout->addWidget(m_zoomToObjectCheckBox);
    
    m_statusLabel = new QLabel("Ready to create mosaic", this);
    resultsLayout->addWidget(m_statusLabel);
    
    m_previewLabel = new QLabel(this);
    m_previewLabel->setMinimumSize(400, 400);
    m_previewLabel->setMaximumSize(400, 400);
    m_previewLabel->setScaledContents(true);
    m_previewLabel->setStyleSheet("border: 1px solid gray; background-color: black;");
    m_previewLabel->setText("Mosaic preview will appear here");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout* previewLayout = new QHBoxLayout();
    previewLayout->addStretch();
    previewLayout->addWidget(m_previewLabel);
    previewLayout->addStretch();
    resultsLayout->addLayout(previewLayout);
    
    mainLayout->addWidget(resultsGroup);
    
    // Connect tab change signal AFTER everything is set up
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MinimalEnhancedMosaicCreator::onTabChanged);
    
    // Initialize with first Messier object
    QTimer::singleShot(0, this, &MinimalEnhancedMosaicCreator::onObjectSelectionChanged);
}

void MinimalEnhancedMosaicCreator::setupMessierTab() {
    QWidget* messierTab = new QWidget();
    QVBoxLayout* messierLayout = new QVBoxLayout(messierTab);
    
    // Object selection (unchanged from original)
    QGroupBox* selectionGroup = new QGroupBox("Messier Object Selection", messierTab);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);
    
    QHBoxLayout* selectorLayout = new QHBoxLayout();
    selectorLayout->addWidget(new QLabel("Select Object:", messierTab));
    
    m_objectSelector = new QComboBox(messierTab);
    m_objectSelector->setMinimumWidth(350);
    
    // Populate with Messier objects
    QStringList objectNames = MessierCatalog::getObjectNames();
    for (const QString& name : objectNames) {
        m_objectSelector->addItem(name);
    }
    
    connect(m_objectSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MinimalEnhancedMosaicCreator::onObjectSelectionChanged);
    
    m_createButton = new QPushButton("Create Messier Mosaic", messierTab);
    connect(m_createButton, &QPushButton::clicked, this, &MinimalEnhancedMosaicCreator::onCreateMosaicClicked);
    
    selectorLayout->addWidget(m_objectSelector);
    selectorLayout->addWidget(m_createButton);
    selectorLayout->addStretch();
    
    selectionLayout->addLayout(selectorLayout);
    
    // Object info display (unchanged from original)
    m_objectInfoLabel = new QLabel("Select an object above", messierTab);
    m_objectInfoLabel->setFont(QFont("Arial", 12, QFont::Bold));
    selectionLayout->addWidget(m_objectInfoLabel);
    
    m_objectDetails = new QTextEdit(messierTab);
    m_objectDetails->setMaximumHeight(150);
    m_objectDetails->setReadOnly(true);
    selectionLayout->addWidget(m_objectDetails);
    
    messierLayout->addWidget(selectionGroup);
    messierLayout->addStretch();
    
    m_tabWidget->addTab(messierTab, "Messier Objects");
}

void MinimalEnhancedMosaicCreator::setupCustomTab() {
    QWidget* customTab = new QWidget();
    QVBoxLayout* customLayout = new QVBoxLayout(customTab);
    
    // Coordinate entry group (NEW)
    QGroupBox* coordGroup = new QGroupBox("Custom Coordinates", customTab);
    QFormLayout* coordForm = new QFormLayout(coordGroup);
    
    m_raInput = new QLineEdit(customTab);
    m_raInput->setPlaceholderText("e.g., 13h29m52.7s or 13:29:52.7 or 202.47");
    connect(m_raInput, &QLineEdit::textChanged, this, &MinimalEnhancedMosaicCreator::onCoordinatesChanged);
    coordForm->addRow("Right Ascension:", m_raInput);
    
    m_decInput = new QLineEdit(customTab);
    m_decInput->setPlaceholderText("e.g., +47d11m43s or +47:11:43 or 47.195");
    connect(m_decInput, &QLineEdit::textChanged, this, &MinimalEnhancedMosaicCreator::onCoordinatesChanged);
    coordForm->addRow("Declination:", m_decInput);
    
    m_nameInput = new QLineEdit(customTab);
    m_nameInput->setPlaceholderText("Object name (optional)");
    m_nameInput->setText("Custom Target");
    coordForm->addRow("Target Name:", m_nameInput);
    
    customLayout->addWidget(coordGroup);
    
    // Format help (NEW)
    QGroupBox* helpGroup = new QGroupBox("Supported Formats", customTab);
    QVBoxLayout* helpLayout = new QVBoxLayout(helpGroup);
    
    QLabel* formatHelp = new QLabel(
        "<b>RA formats:</b> 13h29m52.7s, 13:29:52.7, 202.47<br>"
        "<b>Dec formats:</b> +47d11m43s, +47:11:43, +47.195<br>"
        "<b>Examples:</b><br>"
        "• M51: RA=13h29m52.7s, Dec=+47d11m43s<br>"
        "• Andromeda: RA=0:42:44, Dec=+41:16:09<br>"
        "• Orion: RA=83.82, Dec=-5.39"
    );
    formatHelp->setWordWrap(true);
    formatHelp->setStyleSheet("QLabel { background-color: #f0f8ff; padding: 8px; border: 1px solid #ccc; }");
    helpLayout->addWidget(formatHelp);
    customLayout->addWidget(helpGroup);
    
    // Coordinate preview (NEW)
    m_coordinatePreview = new QLabel("Enter coordinates above", customTab);
    m_coordinatePreview->setStyleSheet("QLabel { background-color: #f5f5f5; padding: 8px; border: 1px solid #aaa; }");
    m_coordinatePreview->setWordWrap(true);
    customLayout->addWidget(m_coordinatePreview);
    
    // Create button (NEW)
    m_createCustomButton = new QPushButton("Create Custom Mosaic", customTab);
    m_createCustomButton->setEnabled(false);
    connect(m_createCustomButton, &QPushButton::clicked, this, &MinimalEnhancedMosaicCreator::onCreateCustomMosaicClicked);
    customLayout->addWidget(m_createCustomButton);
    
    customLayout->addStretch();
    
    m_tabWidget->addTab(customTab, "Custom Coordinates");
}

void MinimalEnhancedMosaicCreator::onTabChanged(int index) {
    m_usingCustomCoordinates = (index == 1);
    
    if (!m_statusLabel) return; // Safety check
    
    if (m_usingCustomCoordinates) {
        m_statusLabel->setText("Enter custom coordinates to create mosaic");
        if (m_raInput && m_decInput) {
            onCoordinatesChanged();
        }
    } else {
        m_statusLabel->setText("Select a Messier object to create mosaic");
        if (m_objectSelector) {
            onObjectSelectionChanged();
        }
    }
}

void MinimalEnhancedMosaicCreator::onObjectSelectionChanged() {
    if (m_usingCustomCoordinates) return;
    
    int index = m_objectSelector->currentIndex();
    if (index >= 0) {
        auto objects = MessierCatalog::getAllObjects();
        if (index < objects.size()) {
            m_currentObject = objects[index];
            updateObjectInfo();
        }
    }
}

void MinimalEnhancedMosaicCreator::updateObjectInfo() {
    if (!m_objectInfoLabel || !m_objectDetails) return;
    
    QString infoText = QString("%1").arg(m_currentObject.name);
    if (!m_currentObject.common_name.isEmpty()) {
        infoText += QString(" (%1)").arg(m_currentObject.common_name);
    }
    m_objectInfoLabel->setText(infoText);
    
    QString details = QString(
        "Type: %1\n"
        "Constellation: %2\n"
        "Coordinates: RA %3°, Dec %4°\n"
        "Magnitude: %5\n"
        "Size: %6 × %7 arcminutes\n"
        "Best viewed: %8\n\n"
        "%9"
    ).arg(MessierCatalog::objectTypeToString(m_currentObject.object_type))
     .arg(MessierCatalog::constellationToString(m_currentObject.constellation))
     .arg(m_currentObject.sky_position.ra_deg, 0, 'f', 3)
     .arg(m_currentObject.sky_position.dec_deg, 0, 'f', 3)
     .arg(m_currentObject.magnitude, 0, 'f', 1)
     .arg(m_currentObject.size_arcmin.width(), 0, 'f', 1)
     .arg(m_currentObject.size_arcmin.height(), 0, 'f', 1)
     .arg(m_currentObject.best_viewed)
     .arg(m_currentObject.description);
    
    m_objectDetails->setText(details);
}

void MinimalEnhancedMosaicCreator::onCoordinatesChanged() {
    if (!m_usingCustomCoordinates || !m_raInput || !m_decInput || 
        !m_coordinatePreview || !m_createCustomButton) return;
    
    QString raText = m_raInput->text().trimmed();
    QString decText = m_decInput->text().trimmed();
    
    if (raText.isEmpty() || decText.isEmpty()) {
        m_coordinatePreview->setText("Enter both RA and Dec coordinates");
        m_createCustomButton->setEnabled(false);
        return;
    }
    
    try {
        QString name = (m_nameInput && !m_nameInput->text().isEmpty()) ? 
                      m_nameInput->text() : "Custom Target";
        
        m_customTarget = SimpleCoordinateParser::parseCoordinates(raText, decText, name);
        updateCoordinatePreview();
        m_createCustomButton->setEnabled(true);
        
    } catch (...) {
        m_coordinatePreview->setText("Invalid coordinate format");
        m_createCustomButton->setEnabled(false);
    }
}

void MinimalEnhancedMosaicCreator::updateCoordinatePreview() {
    if (!m_coordinatePreview || !m_hipsClient) return;
    
    if (m_customTarget.ra_deg < 0 || m_customTarget.ra_deg >= 360 ||
        m_customTarget.dec_deg < -90 || m_customTarget.dec_deg > 90) {
        m_coordinatePreview->setText("Coordinates out of valid range");
        m_createCustomButton->setEnabled(false);
        return;
    }
    
    QString previewText = QString(
        "✅ <b>Parsed Coordinates:</b><br>"
        "Target: %1<br>"
        "RA: %2° Dec: %3°<br>"
        "Ready to create mosaic!"
    ).arg(m_customTarget.name)
     .arg(m_customTarget.ra_deg, 0, 'f', 4)
     .arg(m_customTarget.dec_deg, 0, 'f', 4);
    
    m_coordinatePreview->setText(previewText);
}

void MinimalEnhancedMosaicCreator::onCreateMosaicClicked() {
    if (m_currentObject.name.isEmpty()) return;
    
    m_createButton->setEnabled(false);
    createMosaic(m_currentObject);
}

void MinimalEnhancedMosaicCreator::onCreateCustomMosaicClicked() {
    if (m_customTarget.name.isEmpty()) return;
    
    m_createCustomButton->setEnabled(false);
    createCustomMosaic(m_customTarget);
}

// UNCHANGED: Keep the exact working implementation from main_messier_mosaic.cpp
void MinimalEnhancedMosaicCreator::createMosaic(const MessierObject& messierObj) {
    qDebug() << QString("\n=== Creating Mosaic for %1 ===").arg(messierObj.name);
    
    m_statusLabel->setText(QString("Creating mosaic for %1...").arg(messierObj.name));
    
    createTileGrid(messierObj.sky_position);
    
    qDebug() << QString("Starting download of %1 tiles...").arg(m_tiles.size());
    m_currentTileIndex = 0;
    processNextTile();
}

// NEW: Create custom mosaic (same logic as Messier, different input)
void MinimalEnhancedMosaicCreator::createCustomMosaic(const SkyPosition& target) {
    qDebug() << QString("\n=== Creating Custom Mosaic for %1 ===").arg(target.name);
    
    m_statusLabel->setText(QString("Creating mosaic for %1...").arg(target.name));
    
    createTileGrid(target);
    
    qDebug() << QString("Starting download of %1 tiles...").arg(m_tiles.size());
    m_currentTileIndex = 0;
    processNextTile();
}

// UNCHANGED: Keep the exact working implementation from main_messier_mosaic.cpp
void MinimalEnhancedMosaicCreator::createTileGrid(const SkyPosition& position) {
    m_tiles.clear();
    int order = 8; // Keep the working order
    
    // Calculate 3x3 grid around the target position (UNCHANGED)
    long long centerPixel = m_hipsClient->calculateHealPixel(position, order);
    QList<QList<long long>> grid = m_hipsClient->createProper3x3Grid(centerPixel, order);
    
    qDebug() << QString("Creating 3×3 tile grid for %1:").arg(position.name);
    
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            SimpleTile tile;
            tile.gridX = x;
            tile.gridY = y;
            tile.healpixPixel = grid[y][x];
            tile.downloaded = false;
            
            // Build filename and URL (UNCHANGED)
            QString objectName = position.name.toLower();
            tile.filename = QString("%1/%2_tile_%3_%4_pixel%5.jpg")
                           .arg(m_outputDir).arg(objectName).arg(x).arg(y).arg(tile.healpixPixel);
            
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
    
    qDebug() << QString("Created %1 tile grid for %2").arg(m_tiles.size()).arg(position.name);
}

// UNCHANGED: Keep all the remaining working methods exactly as they were
void MinimalEnhancedMosaicCreator::processNextTile() {
    if (m_currentTileIndex >= m_tiles.size()) {
        assembleFinalMosaic();
        return;
    }
    
    SimpleTile& tile = m_tiles[m_currentTileIndex];
    if (checkExistingTile(tile)) {
        qDebug() << QString("✓ Using existing tile %1/%2: %3")
                    .arg(m_currentTileIndex + 1).arg(m_tiles.size())
                    .arg(QFileInfo(tile.filename).fileName());
        
        m_currentTileIndex++;
        QTimer::singleShot(100, this, &MinimalEnhancedMosaicCreator::processNextTile);
        return;
    }
    
    downloadTile(m_currentTileIndex);
}

void MinimalEnhancedMosaicCreator::downloadTile(int tileIndex) {
    if (tileIndex >= m_tiles.size()) return;
    
    const SimpleTile& tile = m_tiles[tileIndex];
    
    qDebug() << QString("Downloading tile %1/%2: Grid(%3,%4) HEALPix %5")
                .arg(tileIndex + 1).arg(m_tiles.size())
                .arg(tile.gridX).arg(tile.gridY)
                .arg(tile.healpixPixel);
    
    QNetworkRequest request(QUrl(tile.url));
    request.setHeader(QNetworkRequest::UserAgentHeader, "MinimalEnhancedMosaicCreator/1.0");
    request.setRawHeader("Accept", "image/*");
    
    m_downloadStartTime = QDateTime::currentDateTime();
    QNetworkReply* reply = m_networkManager->get(request);
    
    reply->setProperty("tileIndex", tileIndex);
    connect(reply, &QNetworkReply::finished, this, &MinimalEnhancedMosaicCreator::onTileDownloaded);
    
    QTimer::singleShot(15000, reply, &QNetworkReply::abort);
}

void MinimalEnhancedMosaicCreator::onTileDownloaded() {
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
                        .arg(saved ? ", saved" : ", save failed");
        }
    } else {
        qDebug() << QString("❌ Tile %1/%2 download failed: %3")
                    .arg(tileIndex + 1).arg(m_tiles.size())
                    .arg(reply->errorString());
    }
    
    reply->deleteLater();
    m_currentTileIndex++;
    QTimer::singleShot(500, this, &MinimalEnhancedMosaicCreator::processNextTile);
}

void MinimalEnhancedMosaicCreator::assembleFinalMosaic() {
    QString targetName = m_usingCustomCoordinates ? m_customTarget.name : m_currentObject.name;
    
    qDebug() << QString("\n=== Assembling %1 Mosaic ===").arg(targetName);
    
    int successfulTiles = 0;
    for (const SimpleTile& tile : m_tiles) {
        if (tile.downloaded && !tile.image.isNull()) {
            successfulTiles++;
        }
    }
    
    if (successfulTiles == 0) {
        m_statusLabel->setText(QString("Failed to download tiles for %1").arg(targetName));
        m_createButton->setEnabled(true);
        m_createCustomButton->setEnabled(true);
        return;
    }
    
    // Create 3x3 mosaic: 3*512 = 1536 pixels
    int tileSize = 512;
    int mosaicSize = 3 * tileSize;
    
    QImage finalMosaic(mosaicSize, mosaicSize, QImage::Format_RGB32);
    finalMosaic.fill(Qt::black);
    
    QPainter painter(&finalMosaic);
    
    qDebug() << QString("Placing tiles for %1 in 3x3 grid:").arg(targetName);
    
    for (const SimpleTile& tile : m_tiles) {
        if (!tile.downloaded || tile.image.isNull()) {
            qDebug() << QString("  Skipping tile %1,%2 - not downloaded").arg(tile.gridX).arg(tile.gridY);
            continue;
        }
        
        int pixelX = tile.gridX * tileSize;
        int pixelY = tile.gridY * tileSize;
        
        painter.drawImage(pixelX, pixelY, tile.image);
        
        qDebug() << QString("  ✅ Placed tile (%1,%2) at pixel (%3,%4)")
                    .arg(tile.gridX).arg(tile.gridY).arg(pixelX).arg(pixelY);
    }
    
    // Add crosshairs and label at center
    painter.setPen(QPen(Qt::yellow, 3));
    int centerX = mosaicSize / 2;
    int centerY = mosaicSize / 2;
    
    painter.drawLine(centerX - 30, centerY, centerX + 30, centerY);
    painter.drawLine(centerX, centerY - 30, centerX, centerY + 30);
    
    painter.setPen(QPen(Qt::yellow, 1));
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    
    QString labelText = targetName;
    if (!m_usingCustomCoordinates && !m_currentObject.common_name.isEmpty()) {
        labelText = m_currentObject.common_name;
    }
    painter.drawText(centerX + 40, centerY - 10, labelText);
    
    painter.setFont(QFont("Arial", 10));
    SkyPosition currentPos = m_usingCustomCoordinates ? m_customTarget : m_currentObject.sky_position;
    QString coordText = QString("RA:%1° Dec:%2°")
                       .arg(currentPos.ra_deg, 0, 'f', 3)
                       .arg(currentPos.dec_deg, 0, 'f', 3);
    painter.drawText(centerX + 40, centerY + 10, coordText);
    
    painter.end();
    
    // Store the full mosaic
    m_fullMosaic = finalMosaic;
    
    // Save final mosaic
    QString safeName = targetName.toLower().replace(" ", "_").replace("(", "").replace(")", "");
    QString mosaicFilename = QString("%1/%2_mosaic_3x3.png").arg(m_outputDir).arg(safeName);
    bool saved = finalMosaic.save(mosaicFilename);
    
    qDebug() << QString("\n🖼️  %1 mosaic complete!").arg(targetName);
    qDebug() << QString("📁 Size: %1×%2 pixels (%3 tiles placed)")
                .arg(mosaicSize).arg(mosaicSize).arg(successfulTiles);
    qDebug() << QString("📁 Saved to: %1 (%2)")
                .arg(mosaicFilename).arg(saved ? "SUCCESS" : "FAILED");
    
    // Update preview
    updatePreviewDisplay();
    
    QString previewFilename = QString("%1/%2_preview.jpg").arg(m_outputDir).arg(safeName);
    QImage preview512 = finalMosaic.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    preview512.save(previewFilename);
    
    saveProgressReport(targetName);
    
    m_statusLabel->setText(QString("✅ %1 mosaic complete! (%2 tiles)")
                          .arg(targetName).arg(successfulTiles));
    
    qDebug() << QString("\n🎯 %1 MOSAIC COMPLETE!").arg(targetName);
    
    m_createButton->setEnabled(true);
    m_createCustomButton->setEnabled(true);
}

void MinimalEnhancedMosaicCreator::updatePreviewDisplay() {
    if (m_fullMosaic.isNull()) return;
    
    QImage displayImage;
    
    if (m_zoomToObjectCheckBox->isChecked()) {
        displayImage = createZoomedView(m_fullMosaic);
    } else {
        displayImage = m_fullMosaic;
    }
    
    QPixmap preview = QPixmap::fromImage(displayImage.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_previewLabel->setPixmap(preview);
}

QImage MinimalEnhancedMosaicCreator::createZoomedView(const QImage& fullMosaic) {
    if (fullMosaic.isNull()) return QImage();
    
    QPoint actualCenter = findBrightnessCenter(fullMosaic);
    
    // Use real plate solve data: 1.61 arcsec/pixel, 41.2 arcmin total field
    const double TOTAL_FIELD_ARCMIN = 41.2;
    
    double objectSize = 10.0; // Default for custom targets
    if (!m_usingCustomCoordinates) {
        objectSize = std::max(m_currentObject.size_arcmin.width(), m_currentObject.size_arcmin.height());
    }
    
    // Calculate adaptive padding
    double paddingFactor = (objectSize < 3.0) ? 5.0 : (objectSize < 8.0) ? 3.0 : 2.0;
    double paddedObjectSize = objectSize * paddingFactor;
    
    double zoomFraction = paddedObjectSize / TOTAL_FIELD_ARCMIN;
    zoomFraction = std::max(0.3, std::min(1.0, zoomFraction));
    
    int cropSize = static_cast<int>(std::min(fullMosaic.width(), fullMosaic.height()) * zoomFraction);
    
    int cropX = actualCenter.x() - cropSize / 2;
    int cropY = actualCenter.y() - cropSize / 2;
    
    cropX = std::max(0, std::min(cropX, fullMosaic.width() - cropSize));
    cropY = std::max(0, std::min(cropY, fullMosaic.height() - cropSize));
    
    return fullMosaic.copy(QRect(cropX, cropY, cropSize, cropSize));
}

QPoint MinimalEnhancedMosaicCreator::findBrightnessCenter(const QImage& image) {
    if (image.isNull()) return QPoint(image.width()/2, image.height()/2);
    
    QImage blurred = applyGaussianBlur(image, 3);
    
    int width = blurred.width();
    int height = blurred.height();
    
    int maxBrightness = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int brightness = qGray(blurred.pixel(x, y));
            maxBrightness = std::max(maxBrightness, brightness);
        }
    }
    
    if (maxBrightness == 0) return QPoint(width/2, height/2);
    
    double totalWeightedX = 0.0;
    double totalWeightedY = 0.0;
    double totalWeight = 0.0;
    
    int brightnessThreshold = static_cast<int>(maxBrightness * 0.7);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int brightness = qGray(blurred.pixel(x, y));
            
            if (brightness > brightnessThreshold) {
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
        center = QPoint(width/2, height/2);
    }
    
    center.setX(std::max(0, std::min(center.x(), width - 1)));
    center.setY(std::max(0, std::min(center.y(), height - 1)));
    
    return center;
}

QImage MinimalEnhancedMosaicCreator::applyGaussianBlur(const QImage& image, int radius) {
    if (image.isNull() || radius <= 0) return image;
    
    QImage result = image.copy();
    int width = result.width();
    int height = result.height();
    
    // Horizontal blur
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
    
    // Vertical blur
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

bool MinimalEnhancedMosaicCreator::checkExistingTile(const SimpleTile& tile) {
    QFileInfo fileInfo(tile.filename);
    if (!fileInfo.exists() || fileInfo.size() < 1024) return false;
    
    if (!isValidJpeg(tile.filename)) return false;
    
    SimpleTile* mutableTile = const_cast<SimpleTile*>(&tile);
    mutableTile->image.load(tile.filename);
    
    if (mutableTile->image.isNull()) return false;
    
    mutableTile->downloaded = true;
    return true;
}

bool MinimalEnhancedMosaicCreator::isValidJpeg(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray header = file.read(4);
    file.close();
    
    return (header.size() >= 3 && 
            static_cast<unsigned char>(header[0]) == 0xFF && 
            static_cast<unsigned char>(header[1]) == 0xD8 && 
            static_cast<unsigned char>(header[2]) == 0xFF);
}

void MinimalEnhancedMosaicCreator::saveProgressReport(const QString& targetName) {
    QString safeName = targetName.toLower().replace(" ", "_").replace("(", "").replace(")", "");
    QString reportFile = QString("%1/%2_report.txt").arg(m_outputDir).arg(safeName);
    QFile file(reportFile);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    
    QTextStream out(&file);
    out << QString("%1 Mosaic Report\n").arg(targetName);
    out << "Generated: " << QDateTime::currentDateTime().toString() << "\n\n";
    
    if (m_usingCustomCoordinates) {
        out << QString("Custom Target: %1\n").arg(m_customTarget.name);
        out << QString("Coordinates: RA %1°, Dec %2°\n")
               .arg(m_customTarget.ra_deg, 0, 'f', 4)
               .arg(m_customTarget.dec_deg, 0, 'f', 4);
    } else {
        out << QString("Messier Object: %1").arg(m_currentObject.name);
        if (!m_currentObject.common_name.isEmpty()) {
            out << QString(" (%1)").arg(m_currentObject.common_name);
        }
        out << "\n";
        out << QString("Type: %1\n").arg(MessierCatalog::objectTypeToString(m_currentObject.object_type));
        out << QString("Coordinates: RA %1°, Dec %2°\n")
               .arg(m_currentObject.sky_position.ra_deg, 0, 'f', 3)
               .arg(m_currentObject.sky_position.dec_deg, 0, 'f', 3);
    }
    
    out << "\n3x3 Grid Layout:\n";
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
}

// Add missing constellation function
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
    
    qDebug() << "=== Minimal Enhanced Mosaic Creator ===";
    qDebug() << "Based on working Messier version + coordinate input";
    qDebug() << "All core algorithms unchanged!\n";
    
    MinimalEnhancedMosaicCreator creator;
    creator.show();
    
    return app.exec();
}

#include "main_enhanced_mosaic.moc"
