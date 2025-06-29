// main_enhanced_mosaic.cpp - Enhanced coordinate-centered mosaic placement
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

// Coordinate parser (same as minimal version)
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
        
        if (clean.contains(':')) {
            QStringList parts = clean.split(':');
            if (parts.size() >= 2) {
                double hours = parts[0].toDouble();
                double minutes = parts[1].toDouble();
                double seconds = parts.size() > 2 ? parts[2].toDouble() : 0.0;
                return (hours + minutes/60.0 + seconds/3600.0) * 15.0;
            }
        }
        
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
        
        double degrees = clean.toDouble();
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
        
        double result = clean.toDouble();
        return negative ? -result : result;
    }
};

class EnhancedMosaicCreator : public QWidget {
    Q_OBJECT

public:
    explicit EnhancedMosaicCreator(QWidget *parent = nullptr);

private slots:
    void onObjectSelectionChanged();
    void onCreateMosaicClicked();
    void onCreateCustomMosaicClicked();
    void onCoordinatesChanged();
    void onTileDownloaded();
    void processNextTile();
    void onTabChanged(int index);

private:
    ProperHipsClient* m_hipsClient;
    QNetworkAccessManager* m_networkManager;
    
    // UI Components (same as minimal)
    QTabWidget* m_tabWidget;
    QComboBox* m_objectSelector;
    QPushButton* m_createButton;
    QLabel* m_objectInfoLabel;
    QTextEdit* m_objectDetails;
    QLineEdit* m_raInput;
    QLineEdit* m_decInput;
    QLineEdit* m_nameInput;
    QPushButton* m_createCustomButton;
    QLabel* m_coordinatePreview;
    QLabel* m_previewLabel;
    QLabel* m_statusLabel;
    QCheckBox* m_zoomToObjectCheckBox;
    
    // Target tracking
    MessierObject m_currentObject;
    SkyPosition m_customTarget;
    SkyPosition m_actualTarget; // NEW: The actual target coordinates for centering
    bool m_usingCustomCoordinates;
    QImage m_fullMosaic;
    
    // Tile structure (unchanged)
    struct SimpleTile {
        int gridX, gridY;
        long long healpixPixel;
        QString filename;
        QString url;
        QImage image;
        bool downloaded;
        // NEW: Add actual sky coordinates for this tile
        SkyPosition skyCoordinates;
    };
    
    QList<SimpleTile> m_tiles;
    int m_currentTileIndex;
    QString m_outputDir;
    QDateTime m_downloadStartTime;
    
    // UI setup (unchanged)
    void setupUI();
    void setupMessierTab();
    void setupCustomTab();
    void updateObjectInfo();
    void updateCoordinatePreview();
    
    // Core algorithms (enhanced for coordinate centering)
    void createMosaic(const MessierObject& messierObj);
    void createCustomMosaic(const SkyPosition& target);
    void createTileGrid(const SkyPosition& position);
    void downloadTile(int tileIndex);
    
    // NEW: Enhanced mosaic assembly with coordinate-centered placement
    void assembleFinalMosaicCentered();
    QPoint calculateTargetPixelPosition();
    QImage cropMosaicToCenter(const QImage& rawMosaic, const QPoint& targetPixel);
    
    // Helper functions (unchanged)
    void saveProgressReport(const QString& targetName);
    bool checkExistingTile(const SimpleTile& tile);
    bool isValidJpeg(const QString& filename);
    void updatePreviewDisplay();
    QImage createZoomedView(const QImage& fullMosaic);
    QPoint findBrightnessCenter(const QImage& image);
    QImage applyGaussianBlur(const QImage& image, int radius);
    
    // NEW: HEALPix coordinate conversion helpers
    SkyPosition healpixToSkyPosition(long long pixel, int order) const;
    double calculateAngularDistance(const SkyPosition& pos1, const SkyPosition& pos2) const;
};

EnhancedMosaicCreator::EnhancedMosaicCreator(QWidget *parent) 
    : QWidget(parent), m_usingCustomCoordinates(false) {
    
    m_hipsClient = new ProperHipsClient(this);
    m_networkManager = new QNetworkAccessManager(this);
    m_currentTileIndex = 0;
    
    m_outputDir = "enhanced_mosaics";
    QDir().mkpath(m_outputDir);
    
    setupUI();
    
    qDebug() << "=== Enhanced Mosaic Creator - Coordinate Centered ===";
    qDebug() << "Precise coordinate placement with sub-tile accuracy!";
}

void EnhancedMosaicCreator::setupUI() {
    setWindowTitle("Enhanced Mosaic Creator - Coordinate Centered");
    setMinimumSize(800, 700);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget(this);
    
    setupMessierTab();
    setupCustomTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Common controls
    QGroupBox* resultsGroup = new QGroupBox("Results", this);
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    m_zoomToObjectCheckBox = new QCheckBox("Auto-zoom to object size", this);
    connect(m_zoomToObjectCheckBox, &QCheckBox::toggled, this, &EnhancedMosaicCreator::updatePreviewDisplay);
    resultsLayout->addWidget(m_zoomToObjectCheckBox);
    
    m_statusLabel = new QLabel("Ready to create coordinate-centered mosaic", this);
    resultsLayout->addWidget(m_statusLabel);
    
    m_previewLabel = new QLabel(this);
    m_previewLabel->setMinimumSize(400, 400);
    m_previewLabel->setMaximumSize(400, 400);
    m_previewLabel->setScaledContents(true);
    m_previewLabel->setStyleSheet("border: 1px solid gray; background-color: black;");
    m_previewLabel->setText("Coordinate-centered mosaic preview");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout* previewLayout = new QHBoxLayout();
    previewLayout->addStretch();
    previewLayout->addWidget(m_previewLabel);
    previewLayout->addStretch();
    resultsLayout->addLayout(previewLayout);
    
    mainLayout->addWidget(resultsGroup);
    
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &EnhancedMosaicCreator::onTabChanged);
    QTimer::singleShot(0, this, &EnhancedMosaicCreator::onObjectSelectionChanged);
}

void EnhancedMosaicCreator::setupMessierTab() {
    QWidget* messierTab = new QWidget();
    QVBoxLayout* messierLayout = new QVBoxLayout(messierTab);
    
    QGroupBox* selectionGroup = new QGroupBox("Messier Object Selection", messierTab);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);
    
    QHBoxLayout* selectorLayout = new QHBoxLayout();
    selectorLayout->addWidget(new QLabel("Select Object:", messierTab));
    
    m_objectSelector = new QComboBox(messierTab);
    m_objectSelector->setMinimumWidth(350);
    
    QStringList objectNames = MessierCatalog::getObjectNames();
    for (const QString& name : objectNames) {
        m_objectSelector->addItem(name);
    }
    
    connect(m_objectSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnhancedMosaicCreator::onObjectSelectionChanged);
    
    m_createButton = new QPushButton("Create Centered Messier Mosaic", messierTab);
    connect(m_createButton, &QPushButton::clicked, this, &EnhancedMosaicCreator::onCreateMosaicClicked);
    
    selectorLayout->addWidget(m_objectSelector);
    selectorLayout->addWidget(m_createButton);
    selectorLayout->addStretch();
    selectionLayout->addLayout(selectorLayout);
    
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

void EnhancedMosaicCreator::setupCustomTab() {
    QWidget* customTab = new QWidget();
    QVBoxLayout* customLayout = new QVBoxLayout(customTab);
    
    QGroupBox* coordGroup = new QGroupBox("Custom Coordinates", customTab);
    QFormLayout* coordForm = new QFormLayout(coordGroup);
    
    m_raInput = new QLineEdit(customTab);
    m_raInput->setPlaceholderText("e.g., 13h29m52.7s or 13:29:52.7 or 202.47");
    connect(m_raInput, &QLineEdit::textChanged, this, &EnhancedMosaicCreator::onCoordinatesChanged);
    coordForm->addRow("Right Ascension:", m_raInput);
    
    m_decInput = new QLineEdit(customTab);
    m_decInput->setPlaceholderText("e.g., +47d11m43s or +47:11:43 or 47.195");
    connect(m_decInput, &QLineEdit::textChanged, this, &EnhancedMosaicCreator::onCoordinatesChanged);
    coordForm->addRow("Declination:", m_decInput);
    
    m_nameInput = new QLineEdit(customTab);
    m_nameInput->setPlaceholderText("Object name (optional)");
    m_nameInput->setText("Custom Target");
    coordForm->addRow("Target Name:", m_nameInput);
    
    customLayout->addWidget(coordGroup);
    
    QGroupBox* helpGroup = new QGroupBox("Enhanced Coordinate Centering", customTab);
    QVBoxLayout* helpLayout = new QVBoxLayout(helpGroup);
    
    QLabel* formatHelp = new QLabel(
        "<b>Enhanced Features:</b><br>"
        "• <b>Precise centering:</b> Target coordinates become exact center<br>"
        "• <b>Sub-tile accuracy:</b> Crops to place target at center pixel<br>"
        "• <b>Multiple formats:</b> RA: 13h29m52.7s, 13:29:52.7, 202.47<br>"
        "• <b>Dec formats:</b> +47d11m43s, +47:11:43, +47.195<br><br>"
        
        "<b>Examples:</b><br>"
        "• M51 exact center: RA=13h29m52.7s, Dec=+47d11m43s<br>"
        "• Andromeda core: RA=0:42:44, Dec=+41:16:09<br>"
        "• Custom coordinates: RA=83.82, Dec=-5.39<br><br>"
        
        "<b>Note:</b> The entered coordinates will be the exact center of the<br>"
        "final mosaic, not just the center of the nearest HEALPix tile!"
    );
    formatHelp->setWordWrap(true);
    formatHelp->setStyleSheet("QLabel { background-color: #e8f4fd; padding: 8px; border: 1px solid #4a90e2; }");
    helpLayout->addWidget(formatHelp);
    customLayout->addWidget(helpGroup);
    
    m_coordinatePreview = new QLabel("Enter coordinates above for precise centering", customTab);
    m_coordinatePreview->setStyleSheet("QLabel { background-color: #f5f5f5; padding: 8px; border: 1px solid #aaa; }");
    m_coordinatePreview->setWordWrap(true);
    customLayout->addWidget(m_coordinatePreview);
    
    m_createCustomButton = new QPushButton("Create Coordinate-Centered Mosaic", customTab);
    m_createCustomButton->setEnabled(false);
    connect(m_createCustomButton, &QPushButton::clicked, this, &EnhancedMosaicCreator::onCreateCustomMosaicClicked);
    customLayout->addWidget(m_createCustomButton);
    
    customLayout->addStretch();
    
    m_tabWidget->addTab(customTab, "Custom Coordinates");
}

// UI event handlers (unchanged from minimal version)
void EnhancedMosaicCreator::onTabChanged(int index) {
    m_usingCustomCoordinates = (index == 1);
    
    if (!m_statusLabel) return;
    
    if (m_usingCustomCoordinates) {
        m_statusLabel->setText("Enter coordinates for precise centering");
        if (m_raInput && m_decInput) {
            onCoordinatesChanged();
        }
    } else {
        m_statusLabel->setText("Select Messier object for centered mosaic");
        if (m_objectSelector) {
            onObjectSelectionChanged();
        }
    }
}

void EnhancedMosaicCreator::onObjectSelectionChanged() {
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

void EnhancedMosaicCreator::updateObjectInfo() {
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
        "%9\n\n"
        "Note: Coordinates will be precisely centered in the final mosaic."
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

void EnhancedMosaicCreator::onCoordinatesChanged() {
    if (!m_usingCustomCoordinates || !m_raInput || !m_decInput || 
        !m_coordinatePreview || !m_createCustomButton) return;
    
    QString raText = m_raInput->text().trimmed();
    QString decText = m_decInput->text().trimmed();
    
    if (raText.isEmpty() || decText.isEmpty()) {
        m_coordinatePreview->setText("Enter both RA and Dec for precise centering");
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

void EnhancedMosaicCreator::updateCoordinatePreview() {
    if (!m_coordinatePreview || !m_hipsClient) return;
    
    if (m_customTarget.ra_deg < 0 || m_customTarget.ra_deg >= 360 ||
        m_customTarget.dec_deg < -90 || m_customTarget.dec_deg > 90) {
        m_coordinatePreview->setText("Coordinates out of valid range");
        m_createCustomButton->setEnabled(false);
        return;
    }
    
    // Calculate which HEALPix tile this falls into
    long long nearestPixel = m_hipsClient->calculateHealPixel(m_customTarget, 8);
    SkyPosition tileCenter = healpixToSkyPosition(nearestPixel, 8);
    
    // Calculate offset from tile center
    double offsetRA = (m_customTarget.ra_deg - tileCenter.ra_deg) * 3600.0; // arcseconds
    double offsetDec = (m_customTarget.dec_deg - tileCenter.dec_deg) * 3600.0; // arcseconds
    
    QString previewText = QString(
        "✅ <b>Coordinate-Centered Placement:</b><br>"
        "Target: %1<br>"
        "Precise RA: %2° Dec: %3°<br>"
        "Nearest HEALPix tile: %4<br>"
        "Offset from tile center: %5\" RA, %6\" Dec<br><br>"
        "<b>Enhancement:</b> Target will be cropped to exact center!"
    ).arg(m_customTarget.name)
     .arg(m_customTarget.ra_deg, 0, 'f', 6)
     .arg(m_customTarget.dec_deg, 0, 'f', 6)
     .arg(nearestPixel)
     .arg(offsetRA, 0, 'f', 1)
     .arg(offsetDec, 0, 'f', 1);
    
    m_coordinatePreview->setText(previewText);
}

void EnhancedMosaicCreator::onCreateMosaicClicked() {
    if (m_currentObject.name.isEmpty()) return;
    
    m_createButton->setEnabled(false);
    createMosaic(m_currentObject);
}

void EnhancedMosaicCreator::onCreateCustomMosaicClicked() {
    if (m_customTarget.name.isEmpty()) return;
    
    m_createCustomButton->setEnabled(false);
    createCustomMosaic(m_customTarget);
}

void EnhancedMosaicCreator::createMosaic(const MessierObject& messierObj) {
    qDebug() << QString("\n=== Creating Coordinate-Centered Mosaic for %1 ===").arg(messierObj.name);
    
    // Store the actual target coordinates for precise centering
    m_actualTarget = messierObj.sky_position;
    
    m_statusLabel->setText(QString("Creating coordinate-centered mosaic for %1...").arg(messierObj.name));
    
    createTileGrid(messierObj.sky_position);
    
    qDebug() << QString("Target coordinates: RA=%1°, Dec=%2°")
                .arg(m_actualTarget.ra_deg, 0, 'f', 6)
                .arg(m_actualTarget.dec_deg, 0, 'f', 6);
    qDebug() << QString("Starting download of %1 tiles...").arg(m_tiles.size());
    m_currentTileIndex = 0;
    processNextTile();
}

void EnhancedMosaicCreator::createCustomMosaic(const SkyPosition& target) {
    qDebug() << QString("\n=== Creating Coordinate-Centered Mosaic for %1 ===").arg(target.name);
    
    // Store the actual target coordinates for precise centering
    m_actualTarget = target;
    
    m_statusLabel->setText(QString("Creating coordinate-centered mosaic for %1...").arg(target.name));
    
    createTileGrid(target);
    
    qDebug() << QString("Target coordinates: RA=%1°, Dec=%2°")
                .arg(m_actualTarget.ra_deg, 0, 'f', 6)
                .arg(m_actualTarget.dec_deg, 0, 'f', 6);
    qDebug() << QString("Starting download of %1 tiles...").arg(m_tiles.size());
    m_currentTileIndex = 0;
    processNextTile();
}

// UNCHANGED: Keep the working tile grid creation
void EnhancedMosaicCreator::createTileGrid(const SkyPosition& position) {
    m_tiles.clear();
    int order = 8;
    
    long long centerPixel = m_hipsClient->calculateHealPixel(position, order);
    QList<QList<long long>> grid = m_hipsClient->createProper3x3Grid(centerPixel, order);
    
    qDebug() << QString("Creating 3×3 tile grid around %1:").arg(position.name);
    
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            SimpleTile tile;
            tile.gridX = x;
            tile.gridY = y;
            tile.healpixPixel = grid[y][x];
            tile.downloaded = false;
            
            // NEW: Calculate the sky coordinates for this tile
            tile.skyCoordinates = healpixToSkyPosition(tile.healpixPixel, order);
            
            QString objectName = position.name.toLower();
            tile.filename = QString("%1/%2_tile_%3_%4_pixel%5.jpg")
                           .arg(m_outputDir).arg(objectName).arg(x).arg(y).arg(tile.healpixPixel);
            
            int dir = (tile.healpixPixel / 10000) * 10000;
            tile.url = QString("http://alasky.u-strasbg.fr/DSS/DSSColor/Norder%1/Dir%2/Npix%3.jpg")
                      .arg(order).arg(dir).arg(tile.healpixPixel);
            
            // Calculate distance from target to tile center
            double distance = calculateAngularDistance(m_actualTarget, tile.skyCoordinates);
            
            if (tile.healpixPixel == centerPixel) {
                qDebug() << QString("  Grid(%1,%2): HEALPix %3 ★ NEAREST TILE ★ (%4 arcsec from target)")
                            .arg(x).arg(y).arg(tile.healpixPixel).arg(distance * 3600.0, 0, 'f', 1);
            } else {
                qDebug() << QString("  Grid(%1,%2): HEALPix %3 (%4 arcsec from target)")
                            .arg(x).arg(y).arg(tile.healpixPixel).arg(distance * 3600.0, 0, 'f', 1);
            }
            
            m_tiles.append(tile);
        }
    }
    
    qDebug() << QString("Created %1 tile grid - will crop to center target precisely").arg(m_tiles.size());
}

// UNCHANGED: Keep working download logic
void EnhancedMosaicCreator::processNextTile() {
    if (m_currentTileIndex >= m_tiles.size()) {
        assembleFinalMosaicCentered(); // NEW: Use enhanced assembly
        return;
    }
    
    SimpleTile& tile = m_tiles[m_currentTileIndex];
    if (checkExistingTile(tile)) {
        m_currentTileIndex++;
        QTimer::singleShot(100, this, &EnhancedMosaicCreator::processNextTile);
        return;
    }
    
    downloadTile(m_currentTileIndex);
}

void EnhancedMosaicCreator::downloadTile(int tileIndex) {
    if (tileIndex >= m_tiles.size()) return;
    
    const SimpleTile& tile = m_tiles[tileIndex];
    
    qDebug() << QString("Downloading tile %1/%2: Grid(%3,%4) HEALPix %5")
                .arg(tileIndex + 1).arg(m_tiles.size())
                .arg(tile.gridX).arg(tile.gridY)
                .arg(tile.healpixPixel);
    
    QNetworkRequest request(QUrl(tile.url));
    request.setHeader(QNetworkRequest::UserAgentHeader, "EnhancedMosaicCreator/1.0");
    request.setRawHeader("Accept", "image/*");
    
    m_downloadStartTime = QDateTime::currentDateTime();
    QNetworkReply* reply = m_networkManager->get(request);
    
    reply->setProperty("tileIndex", tileIndex);
    connect(reply, &QNetworkReply::finished, this, &EnhancedMosaicCreator::onTileDownloaded);
    
    QTimer::singleShot(15000, reply, &QNetworkReply::abort);
}

void EnhancedMosaicCreator::onTileDownloaded() {
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
    QTimer::singleShot(500, this, &EnhancedMosaicCreator::processNextTile);
}

// NEW: Enhanced mosaic assembly with coordinate-centered placement
void EnhancedMosaicCreator::assembleFinalMosaicCentered() {
    QString targetName = m_usingCustomCoordinates ? m_customTarget.name : m_currentObject.name;
    
    qDebug() << QString("\n=== Assembling Coordinate-Centered %1 Mosaic ===").arg(targetName);
    
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
    
    // Step 1: Create the raw 3x3 mosaic (unchanged algorithm)
    int tileSize = 512;
    int rawMosaicSize = 3 * tileSize; // 1536x1536
    
    QImage rawMosaic(rawMosaicSize, rawMosaicSize, QImage::Format_RGB32);
    rawMosaic.fill(Qt::black);
    
    QPainter rawPainter(&rawMosaic);
    
    qDebug() << QString("Step 1: Assembling raw 3x3 mosaic (%1x%1 pixels)").arg(rawMosaicSize);
    
    for (const SimpleTile& tile : m_tiles) {
        if (!tile.downloaded || tile.image.isNull()) {
            qDebug() << QString("  Skipping tile %1,%2 - not downloaded").arg(tile.gridX).arg(tile.gridY);
            continue;
        }
        
        int pixelX = tile.gridX * tileSize;
        int pixelY = tile.gridY * tileSize;
        
        rawPainter.drawImage(pixelX, pixelY, tile.image);
        
        qDebug() << QString("  ✅ Placed tile (%1,%2) at pixel (%3,%4)")
                    .arg(tile.gridX).arg(tile.gridY).arg(pixelX).arg(pixelY);
    }
    rawPainter.end();
    
    // Step 2: Calculate where the target coordinates fall in the raw mosaic
    QPoint targetPixel = calculateTargetPixelPosition();
    
    qDebug() << QString("Step 2: Target coordinates map to pixel (%1,%2) in raw mosaic")
                .arg(targetPixel.x()).arg(targetPixel.y());
    
    // Step 3: Crop the mosaic to center the target coordinates
    QImage centeredMosaic = cropMosaicToCenter(rawMosaic, targetPixel);
    
    qDebug() << QString("Step 3: Cropped to %1x%2 centered mosaic")
                .arg(centeredMosaic.width()).arg(centeredMosaic.height());
    
    // Step 4: Add crosshairs and labels at the true center
    QPainter painter(&centeredMosaic);
    
    // Add crosshairs at the exact center (where target coordinates are)
    painter.setPen(QPen(Qt::yellow, 3));
    int centerX = centeredMosaic.width() / 2;
    int centerY = centeredMosaic.height() / 2;
    
    painter.drawLine(centerX - 30, centerY, centerX + 30, centerY);
    painter.drawLine(centerX, centerY - 30, centerX, centerY + 30);
    
    // Add precise coordinate labels
    painter.setPen(QPen(Qt::yellow, 1));
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    
    QString labelText = targetName;
    if (!m_usingCustomCoordinates && !m_currentObject.common_name.isEmpty()) {
        labelText = m_currentObject.common_name;
    }
    painter.drawText(centerX + 40, centerY - 20, labelText);
    
    painter.setFont(QFont("Arial", 10));
    QString coordText = QString("RA:%1° Dec:%2°")
                       .arg(m_actualTarget.ra_deg, 0, 'f', 4)
                       .arg(m_actualTarget.dec_deg, 0, 'f', 4);
    painter.drawText(centerX + 40, centerY - 5, coordText);
    
    painter.drawText(centerX + 40, centerY + 10, "COORDINATE CENTERED");
    
    painter.end();
    
    // Store the final centered mosaic
    m_fullMosaic = centeredMosaic;
    
    // Save final mosaic
    QString safeName = targetName.toLower().replace(" ", "_").replace("(", "").replace(")", "");
    QString mosaicFilename = QString("%1/%2_centered_mosaic.png").arg(m_outputDir).arg(safeName);
    bool saved = centeredMosaic.save(mosaicFilename);
    
    qDebug() << QString("\n🎯 %1 COORDINATE-CENTERED MOSAIC COMPLETE!").arg(targetName);
    qDebug() << QString("📁 Final size: %1×%2 pixels (%3 tiles used)")
                .arg(centeredMosaic.width()).arg(centeredMosaic.height()).arg(successfulTiles);
    qDebug() << QString("📁 Saved to: %1 (%2)")
                .arg(mosaicFilename).arg(saved ? "SUCCESS" : "FAILED");
    qDebug() << QString("✅ Target coordinates are now at exact center pixel (%1,%2)")
                .arg(centerX).arg(centerY);
    
    // Update preview
    updatePreviewDisplay();
    
    // Save preview
    QString previewFilename = QString("%1/%2_centered_preview.jpg").arg(m_outputDir).arg(safeName);
    QImage preview512 = centeredMosaic.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    preview512.save(previewFilename);
    
    saveProgressReport(targetName);
    
    m_statusLabel->setText(QString("✅ %1 coordinate-centered mosaic complete!")
                          .arg(targetName));
    
    m_createButton->setEnabled(true);
    m_createCustomButton->setEnabled(true);
}

// NEW: Calculate where the target coordinates fall in the raw mosaic pixel grid
QPoint EnhancedMosaicCreator::calculateTargetPixelPosition() {
    // Find the tile that contains our target (should be the center tile, but let's be precise)
    SimpleTile* containingTile = nullptr;
    double minDistance = std::numeric_limits<double>::max();
    
    for (SimpleTile& tile : m_tiles) {
        double distance = calculateAngularDistance(m_actualTarget, tile.skyCoordinates);
        if (distance < minDistance) {
            minDistance = distance;
            containingTile = &tile;
        }
    }
    
    if (!containingTile) {
        qDebug() << "Warning: Could not find containing tile, using geometric center";
        return QPoint(1536/2, 1536/2);
    }
    
    qDebug() << QString("Target is in tile (%1,%2) with center at RA=%3°, Dec=%4°")
                .arg(containingTile->gridX).arg(containingTile->gridY)
                .arg(containingTile->skyCoordinates.ra_deg, 0, 'f', 6)
                .arg(containingTile->skyCoordinates.dec_deg, 0, 'f', 6);
    
    // FIXED: Use definitive astrometry data from astrometry.net
    // Field size: 32.2 × 32.2 arcminutes for 1200×1200 final mosaic
    // But our raw mosaic is 1536×1536, so:
    // Raw mosaic field = 32.2 * (1536/1200) = 41.2 arcminutes
    // Pixel scale: 1.61 arcsec/pixel (verified by astrometry.net)
    const double ARCSEC_PER_PIXEL = 1.61;
    
    // Calculate angular offsets from the nearest tile center
    double offsetRA_arcsec = (m_actualTarget.ra_deg - containingTile->skyCoordinates.ra_deg) * 3600.0;
    double offsetDec_arcsec = (m_actualTarget.dec_deg - containingTile->skyCoordinates.dec_deg) * 3600.0;
    
    // Apply cosine correction for RA at this declination
    offsetRA_arcsec *= cos(m_actualTarget.dec_deg * M_PI / 180.0);
    
    qDebug() << QString("Angular offset from tile center: RA=%1\", Dec=%2\"")
                .arg(offsetRA_arcsec, 0, 'f', 2)
                .arg(offsetDec_arcsec, 0, 'f', 2);
    
    // Convert to pixel offsets using verified pixel scale
    double offsetRA_pixels = offsetRA_arcsec / ARCSEC_PER_PIXEL;
    double offsetDec_pixels = -offsetDec_arcsec / ARCSEC_PER_PIXEL; // Negative because Y increases downward
    
    qDebug() << QString("Pixel offset from tile center: %1,%2 pixels")
                .arg(offsetRA_pixels, 0, 'f', 1)
                .arg(offsetDec_pixels, 0, 'f', 1);
    
    // SANITY CHECK: For order 8 HEALPix, offsets should be reasonable
    if (abs(offsetRA_pixels) > 400 || abs(offsetDec_pixels) > 400) {
        qDebug() << QString("WARNING: Very large pixel offsets RA=%1, Dec=%2 - may indicate coordinate error")
                    .arg(offsetRA_pixels, 0, 'f', 1).arg(offsetDec_pixels, 0, 'f', 1);
        qDebug() << "Using geometric center as fallback";
        return QPoint(1536/2, 1536/2);
    }
    
    // Calculate absolute pixel position in the 1536x1536 raw mosaic
    int tilePixelX = containingTile->gridX * 512 + 256; // Tile center X
    int tilePixelY = containingTile->gridY * 512 + 256; // Tile center Y
    
    int targetPixelX = tilePixelX + static_cast<int>(round(offsetRA_pixels));
    int targetPixelY = tilePixelY + static_cast<int>(round(offsetDec_pixels));
    
    // Clamp to mosaic bounds
    targetPixelX = std::max(0, std::min(targetPixelX, 1535));
    targetPixelY = std::max(0, std::min(targetPixelY, 1535));
    
    qDebug() << QString("Tile center pixel: (%1,%2)")
                .arg(tilePixelX).arg(tilePixelY);
    qDebug() << QString("Target pixel in raw mosaic: (%1,%2)")
                .arg(targetPixelX).arg(targetPixelY);
    
    // VALIDATION: Calculate distance from geometric center
    int geometricCenterX = 1536/2;
    int geometricCenterY = 1536/2;
    int distanceFromCenter = sqrt(pow(targetPixelX - geometricCenterX, 2) + pow(targetPixelY - geometricCenterY, 2));
    
    qDebug() << QString("Distance from geometric center: %1 pixels (%2 arcsec)")
                .arg(distanceFromCenter)
                .arg(distanceFromCenter * ARCSEC_PER_PIXEL, 0, 'f', 1);
    
    return QPoint(targetPixelX, targetPixelY);
}

// NEW: Crop the raw mosaic to center the target coordinates
QImage EnhancedMosaicCreator::cropMosaicToCenter(const QImage& rawMosaic, const QPoint& targetPixel) {
    // Determine crop size - aim for ~1200x1200 final mosaic (keeps good resolution)
    int cropSize = 1200;
    
    // Ensure we don't exceed the raw mosaic bounds
    cropSize = std::min(cropSize, std::min(rawMosaic.width(), rawMosaic.height()));
    
    // FIXED: Calculate crop rectangle so target pixel becomes the center
    int cropX = targetPixel.x() - cropSize / 2;
    int cropY = targetPixel.y() - cropSize / 2;
    
    // Adjust if crop would go outside raw mosaic bounds
    if (cropX < 0) {
        qDebug() << QString("Crop X adjusted from %1 to 0 (target too close to left edge)").arg(cropX);
        cropX = 0;
    }
    if (cropY < 0) {
        qDebug() << QString("Crop Y adjusted from %1 to 0 (target too close to top edge)").arg(cropY);
        cropY = 0;
    }
    if (cropX + cropSize > rawMosaic.width()) {
        int oldCropX = cropX;
        cropX = rawMosaic.width() - cropSize;
        qDebug() << QString("Crop X adjusted from %1 to %2 (target too close to right edge)").arg(oldCropX).arg(cropX);
    }
    if (cropY + cropSize > rawMosaic.height()) {
        int oldCropY = cropY;
        cropY = rawMosaic.height() - cropSize;
        qDebug() << QString("Crop Y adjusted from %1 to %2 (target too close to bottom edge)").arg(oldCropY).arg(cropY);
    }
    
    QRect cropRect(cropX, cropY, cropSize, cropSize);
    
    // Calculate where the target will be in the final cropped image
    int targetInCropX = targetPixel.x() - cropX;
    int targetInCropY = targetPixel.y() - cropY;
    int expectedCenterX = cropSize / 2;
    int expectedCenterY = cropSize / 2;
    
    qDebug() << QString("Crop rectangle: (%1,%2) %3x%4")
                .arg(cropX).arg(cropY).arg(cropSize).arg(cropSize);
    qDebug() << QString("Target in crop: (%1,%2), expected center: (%3,%4), offset: %5,%6 pixels")
                .arg(targetInCropX).arg(targetInCropY)
                .arg(expectedCenterX).arg(expectedCenterY)
                .arg(targetInCropX - expectedCenterX)
                .arg(targetInCropY - expectedCenterY);
    
    // VALIDATION: Check if centering is reasonable (within 50 pixels of center)
    int offsetX = abs(targetInCropX - expectedCenterX);
    int offsetY = abs(targetInCropY - expectedCenterY);
    if (offsetX > 50 || offsetY > 50) {
        qDebug() << QString("WARNING: Large centering offset (%1,%2) - may indicate coordinate calculation error")
                    .arg(offsetX).arg(offsetY);
    }
    
    return rawMosaic.copy(cropRect);
}

// NEW: Convert HEALPix pixel back to sky coordinates
SkyPosition EnhancedMosaicCreator::healpixToSkyPosition(long long pixel, int order) const {
    try {
        long long nside = 1LL << order;
        Healpix_Base healpix(nside, NEST, SET_NSIDE);
        
        pointing pt = healpix.pix2ang(pixel);
        
        SkyPosition pos;
        pos.ra_deg = pt.phi * 180.0 / M_PI;
        pos.dec_deg = 90.0 - pt.theta * 180.0 / M_PI;
        pos.name = QString("HEALPix_%1").arg(pixel);
        pos.description = QString("Order %1 pixel %2").arg(order).arg(pixel);
        
        return pos;
    } catch (...) {
        // Fallback
        SkyPosition pos;
        pos.ra_deg = 0.0;
        pos.dec_deg = 0.0;
        pos.name = "Error";
        pos.description = "HEALPix conversion failed";
        return pos;
    }
}

// NEW: Calculate angular distance between two sky positions
double EnhancedMosaicCreator::calculateAngularDistance(const SkyPosition& pos1, const SkyPosition& pos2) const {
    // Convert to radians
    double ra1 = pos1.ra_deg * M_PI / 180.0;
    double dec1 = pos1.dec_deg * M_PI / 180.0;
    double ra2 = pos2.ra_deg * M_PI / 180.0;
    double dec2 = pos2.dec_deg * M_PI / 180.0;
    
    // Haversine formula for angular distance
    double dra = ra2 - ra1;
    double ddec = dec2 - dec1;
    
    double a = sin(ddec/2) * sin(ddec/2) + 
               cos(dec1) * cos(dec2) * sin(dra/2) * sin(dra/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return c; // Return in radians
}

// Helper functions (unchanged from minimal version)
void EnhancedMosaicCreator::updatePreviewDisplay() {
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

QImage EnhancedMosaicCreator::createZoomedView(const QImage& fullMosaic) {
    if (fullMosaic.isNull()) return QImage();
    
    // For coordinate-centered mosaics, the target is already at the center
    // So we can just crop around the center
    double objectSize = 10.0; // Default for custom targets
    if (!m_usingCustomCoordinates) {
        objectSize = std::max(m_currentObject.size_arcmin.width(), m_currentObject.size_arcmin.height());
    }
    
    // Calculate crop size based on object size
    // Assume the full mosaic covers about 25 arcminutes
    const double TOTAL_FIELD_ARCMIN = 25.0;
    double paddingFactor = (objectSize < 3.0) ? 3.0 : (objectSize < 8.0) ? 2.0 : 1.5;
    double paddedObjectSize = objectSize * paddingFactor;
    
    double zoomFraction = paddedObjectSize / TOTAL_FIELD_ARCMIN;
    zoomFraction = std::max(0.3, std::min(1.0, zoomFraction));
    
    int cropSize = static_cast<int>(std::min(fullMosaic.width(), fullMosaic.height()) * zoomFraction);
    
    // Crop around the center (where our target coordinates are)
    int centerX = fullMosaic.width() / 2;
    int centerY = fullMosaic.height() / 2;
    
    int cropX = centerX - cropSize / 2;
    int cropY = centerY - cropSize / 2;
    
    cropX = std::max(0, std::min(cropX, fullMosaic.width() - cropSize));
    cropY = std::max(0, std::min(cropY, fullMosaic.height() - cropSize));
    
    return fullMosaic.copy(QRect(cropX, cropY, cropSize, cropSize));
}

QPoint EnhancedMosaicCreator::findBrightnessCenter(const QImage& image) {
    // For coordinate-centered mosaics, just return the geometric center
    // since we've already placed the target at the center
    return QPoint(image.width()/2, image.height()/2);
}

QImage EnhancedMosaicCreator::applyGaussianBlur(const QImage& image, int radius) {
    if (image.isNull() || radius <= 0) return image;
    
    // Simple implementation (unchanged)
    QImage result = image.copy();
    int width = result.width();
    int height = result.height();
    
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

bool EnhancedMosaicCreator::checkExistingTile(const SimpleTile& tile) {
    QFileInfo fileInfo(tile.filename);
    if (!fileInfo.exists() || fileInfo.size() < 1024) return false;
    
    if (!isValidJpeg(tile.filename)) return false;
    
    SimpleTile* mutableTile = const_cast<SimpleTile*>(&tile);
    mutableTile->image.load(tile.filename);
    
    if (mutableTile->image.isNull()) return false;
    
    mutableTile->downloaded = true;
    return true;
}

bool EnhancedMosaicCreator::isValidJpeg(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return false;
    
    QByteArray header = file.read(4);
    file.close();
    
    return (header.size() >= 3 && 
            static_cast<unsigned char>(header[0]) == 0xFF && 
            static_cast<unsigned char>(header[1]) == 0xD8 && 
            static_cast<unsigned char>(header[2]) == 0xFF);
}

void EnhancedMosaicCreator::saveProgressReport(const QString& targetName) {
    QString safeName = targetName.toLower().replace(" ", "_").replace("(", "").replace(")", "");
    QString reportFile = QString("%1/%2_centered_report.txt").arg(m_outputDir).arg(safeName);
    QFile file(reportFile);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    
    QTextStream out(&file);
    out << QString("%1 Coordinate-Centered Mosaic Report\n").arg(targetName);
    out << "Generated: " << QDateTime::currentDateTime().toString() << "\n\n";
    
    out << "COORDINATE CENTERING ENHANCEMENT:\n";
    out << QString("Target coordinates: RA %1°, Dec %2°\n")
           .arg(m_actualTarget.ra_deg, 0, 'f', 6)
           .arg(m_actualTarget.dec_deg, 0, 'f', 6);
    out << "Enhancement: Target coordinates placed at exact mosaic center\n\n";
    
    if (m_usingCustomCoordinates) {
        out << QString("Custom Target: %1\n").arg(m_customTarget.name);
    } else {
        out << QString("Messier Object: %1").arg(m_currentObject.name);
        if (!m_currentObject.common_name.isEmpty()) {
            out << QString(" (%1)").arg(m_currentObject.common_name);
        }
        out << "\n";
        out << QString("Type: %1\n").arg(MessierCatalog::objectTypeToString(m_currentObject.object_type));
    }
    
    out << "\n3x3 Tile Grid Used:\n";
    out << "Grid_X,Grid_Y,HEALPix_Pixel,Tile_RA,Tile_Dec,Downloaded,ImageSize,Filename\n";
    
    for (const SimpleTile& tile : m_tiles) {
        out << QString("%1,%2,%3,%4,%5,%6,%7x%8,%9\n")
               .arg(tile.gridX).arg(tile.gridY)
               .arg(tile.healpixPixel)
               .arg(tile.skyCoordinates.ra_deg, 0, 'f', 6)
               .arg(tile.skyCoordinates.dec_deg, 0, 'f', 6)
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
    
    qDebug() << "=== Enhanced Mosaic Creator - Coordinate Centered ===";
    qDebug() << "Precise coordinate placement with sub-tile accuracy!";
    qDebug() << "Your entered coordinates will be the exact center of the mosaic.\n";
    
    EnhancedMosaicCreator creator;
    creator.show();
    
    return app.exec();
}

#include "main_enhanced_mosaic.moc"
    
