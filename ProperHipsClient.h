// ProperHipsClient.h - Minimal header for successful compilation
#ifndef PROPERHIPSCLIENT_H
#define PROPERHIPSCLIENT_H

#include <QPoint>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QMutex>
#include <QSet>

// Conditional HEALPix includes - only if available
#ifdef HEALPIX_AVAILABLE
#include "healpix_base.h"
#include "pointing.h"
#else
// Minimal fallback definitions if HEALPix not available
namespace {
    struct pointing {
        double theta, phi;
        pointing(double t, double p) : theta(t), phi(p) {}
    };
    
    template<class T, int N>
    struct fix_arr {
        T data[N];
        T& operator[](int i) { return data[i]; }
        const T& operator[](int i) const { return data[i]; }
    };
    
    enum Healpix_Ordering_Scheme { NEST, RING };
    enum { SET_NSIDE };
    
    class Healpix_Base {
    public:
        Healpix_Base(long long nside, Healpix_Ordering_Scheme scheme, int param) 
            : m_nside(nside) { Q_UNUSED(scheme) Q_UNUSED(param) }
        
        long long ang2pix(const pointing& pt) const {
            // Simple fallback calculation
            double z = cos(pt.theta);
            double phi = pt.phi;
            if (phi < 0) phi += 2 * M_PI;
            
            long long npix = 12 * m_nside * m_nside;
            long long pixel = static_cast<long long>((phi / (2 * M_PI)) * npix) % npix;
            return pixel;
        }
        
        void neighbors(long long pixel, fix_arr<int, 8>& result) const {
            // Simple fallback - just return adjacent pixel numbers
            for (int i = 0; i < 8; i++) {
                result[i] = static_cast<int>(pixel + i - 4);
                if (result[i] < 0) result[i] = -1;
            }
        }
        
    private:
        long long m_nside;
    };
}
#endif

struct HipsSurveyInfo {
    QString name;
    QString baseUrl;
    QString format;
    QString description;
    bool available;
    int maxOrder;
    QStringList regions;
};

struct SkyPosition {
    double ra_deg;
    double dec_deg;
    QString name;
    QString description;
    
    // Convert to HEALPix pointing
    pointing toPointing() const {
        double theta = (90.0 - dec_deg) * M_PI / 180.0;  // colatitude
        double phi = ra_deg * M_PI / 180.0;              // longitude
        return pointing(theta, phi);
    }
};

struct TileResult {
    QString survey;
    QString position;
    bool success;
    int httpStatus;
    qint64 downloadTime;
    qint64 fileSize;
    QString url;
    long long healpixPixel;
    int order;
    QDateTime timestamp;
};

class ProperHipsClient : public QObject {
    Q_OBJECT

public:
    explicit ProperHipsClient(QObject *parent = nullptr);
    
    // Main testing interface
    void testAllSurveys();
    void testSurveyAtPosition(const QString& surveyName, const SkyPosition& position);
    void testPixelCalculation();
    void testGridValidation();
    
    // Production interface for telescope simulator
    QStringList getWorkingSurveys() const;
    QString getBestSurveyForPosition(const SkyPosition& position) const;
    QString buildTileUrl(const QString& surveyName, const SkyPosition& position, int order = 6) const;
    
    // Results access
    QList<TileResult> getResults() const { return m_results; }
    void saveResults(const QString& filename) const;
    void printSummary() const;

    // HEALPix grid generation methods
    long long calculateHealPixel(const SkyPosition& position, int order) const;
    QList<long long> getNeighboringPixels(long long centerPixel, int order) const;
    QMap<QString, long long> getDirectionalNeighbors(long long centerPixel, int order) const;
    QList<QList<long long>> createProper3x3Grid(long long centerPixel, int order) const;
    QList<QList<long long>> createProperNxMGrid(long long centerPixel, int order, int gridWidth, int gridHeight) const;
    QList<long long> calculateNeighborRing(long long centerPixel, int order, int ringRadius) const;
                                         
private slots:
    void onReplyFinished();

signals:
    void testingComplete();

private:
    QNetworkAccessManager* m_networkManager;
    QMap<QString, HipsSurveyInfo> m_surveys;
    QList<SkyPosition> m_testPositions;
    QList<TileResult> m_results;
    QTimer* m_testTimer;
    
    // Test state
    int m_currentSurveyIndex;
    int m_currentPositionIndex;
    QDateTime m_requestStartTime;
    
    void setupSurveys();
    void setupTestPositions();
    void startNextTest();
    void moveToNextTest();
    void finishTesting();
    
    // HEALPix utilities
    long long calculateSimplePixel(double ra_deg, double dec_deg, int order) const; // For comparison
    QList<long long> calculateTileGrid(const SkyPosition& center, int order, int gridSize = 4) const;
    
    // URL building for different survey types
    QString buildDSSUrl(const SkyPosition& position, int order, const QString& survey) const;
    QString build2MASSUrl(const SkyPosition& position, int order, const QString& survey) const;
    QString buildRubinUrl(const SkyPosition& position, int order, const QString& survey) const;
    QString buildGenericHipsUrl(const QString& baseUrl, const QString& format, 
                               const SkyPosition& position, int order) const;
    
    // Helper methods for NxM grid generation
    QList<QList<long long>> createFallbackGrid(long long centerPixel, int order, int gridWidth, int gridHeight) const;
    long long findNearestCalculatedPixel(int targetX, int targetY, 
                                       const QMap<QString, long long>& calculatedPixels,
                                       int gridWidth, int gridHeight) const;
};

#endif // PROPERHIPSCLIENT_H