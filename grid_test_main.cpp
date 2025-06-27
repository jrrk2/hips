// grid_test_main.cpp - Focused test for grid generation validation
#include <QCoreApplication>
#include <QDebug>
#include "ProperHipsClient.h"

void testSpecificGrid(ProperHipsClient& client, long long centerPixel, int order, int width, int height) {
    qDebug() << QString("\n=== Testing %1×%2 Grid ===").arg(width).arg(height);
    
    // Generate the grid
    QList<QList<long long>> grid = client.createProperNxMGrid(centerPixel, order, width, height);
    
    if (grid.isEmpty()) {
        qDebug() << "❌ Grid generation failed - empty result";
        return;
    }
    
    if (grid.size() != height) {
        qDebug() << QString("❌ Wrong height: expected %1, got %2").arg(height).arg(grid.size());
        return;
    }
    
    if (grid[0].size() != width) {
        qDebug() << QString("❌ Wrong width: expected %1, got %2").arg(width).arg(grid[0].size());
        return;
    }
    
    // Show the grid layout
    qDebug() << QString("Generated %1×%2 grid:").arg(width).arg(height);
    for (int y = 0; y < height; y++) {
        QString row = QString("  Row %1: ").arg(y);
        for (int x = 0; x < width; x++) {
            row += QString("%1 ").arg(grid[y][x]);
        }
        qDebug() << row;
    }
    
    // Verify center pixel
    int centerX = width / 2;
    int centerY = height / 2;
    long long actualCenter = grid[centerY][centerX];
    
    if (actualCenter == centerPixel) {
        qDebug() << QString("✅ Center pixel correct at (%1,%2): %3").arg(centerX).arg(centerY).arg(actualCenter);
    } else {
        qDebug() << QString("❌ Center pixel wrong at (%1,%2): expected %3, got %4")
                    .arg(centerX).arg(centerY).arg(centerPixel).arg(actualCenter);
    }
    
    // Count valid pixels (non-negative)
    int validPixels = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (grid[y][x] >= 0) {
                validPixels++;
            }
        }
    }
    
    double coverage = double(validPixels) / double(width * height) * 100.0;
    qDebug() << QString("Coverage: %1/%2 pixels valid (%3%)")
                .arg(validPixels).arg(width * height).arg(coverage, 0, 'f', 1);
    
    if (coverage >= 95.0) {
        qDebug() << QString("✅ %1×%2 grid test PASSED").arg(width).arg(height);
    } else {
        qDebug() << QString("⚠️  %1×%2 grid test WARNING - low coverage").arg(width).arg(height);
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Grid Generation Validation Test ===";
    qDebug() << "Testing improved grid generation algorithms\n";
    
    ProperHipsClient client;
    
    // Use M31 as test position (known to work well)
    SkyPosition m31 = {10.6847, 41.2687, "M31", "Andromeda Galaxy"};
    long long centerPixel = client.calculateHealPixel(m31, 8);
    
    qDebug() << QString("Test center: M31 at pixel %1 (order 8)").arg(centerPixel);
    
    if (centerPixel < 0) {
        qDebug() << "❌ Failed to calculate center pixel";
        return 1;
    }
    
    // First, verify the 3×3 reference works
    qDebug() << "\n--- Baseline 3×3 Grid Test ---";
    QList<QList<long long>> reference3x3 = client.createProper3x3Grid(centerPixel, 8);
    
    if (reference3x3.size() == 3 && reference3x3[0].size() == 3) {
        qDebug() << "✅ 3×3 reference grid generation works";
        qDebug() << "Reference layout:";
        for (int y = 0; y < 3; y++) {
            QString row = "  ";
            for (int x = 0; x < 3; x++) {
                row += QString("[%1] ").arg(reference3x3[y][x]);
            }
            qDebug() << row;
        }
    } else {
        qDebug() << "❌ 3×3 reference grid failed";
        return 1;
    }
    
    // Test progressive grid sizes
    QList<QPair<int,int>> testGrids = {
        {3, 3},   // Should match reference exactly
        {4, 4},   // Small extension
        {5, 5},   // Medium extension  
        {6, 6},   // Large extension
        {8, 6},   // Rectangular
        {4, 3},   // Narrow rectangular
        {10, 8},  // Very large
        {15, 10}  // Huge grid
    };
    
    for (const auto& gridSize : testGrids) {
        testSpecificGrid(client, centerPixel, 8, gridSize.first, gridSize.second);
    }
    
    qDebug() << "\n=== Grid Validation Complete ===";
    qDebug() << "If any grids failed, the algorithm needs further refinement.";
    
    return 0;
}