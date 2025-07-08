# Akhanda Engine - Core.Math Test Suite

## Overview

This comprehensive test suite validates the `Core.Math` module of the Akhanda Engine, covering vector mathematics, matrix operations, quaternion calculations, and SIMD optimizations. The test suite is designed to ensure correctness, performance, and reliability across all mathematical operations used throughout the engine.

## Test Organization

### Directory Structure

```
Tests/Core.Math/
├── Source/
│   ├── Main.cpp                     # Test entry point and environment setup
│   ├── TestConstants.hpp            # Mathematical constants and utilities
│   ├── Fixtures/
│   │   └── MathTestFixtures.hpp     # Reusable test fixtures and base classes
│   ├── UnitTests/
│   │   ├── Vector3Tests.cpp         # Vector3 functionality tests
│   │   ├── Vector4Tests.cpp         # Vector4 functionality tests
│   │   ├── Matrix4Tests.cpp         # Matrix4 functionality tests
│   │   ├── QuaternionTests.cpp      # Quaternion functionality tests
│   │   ├── MathUtilsTests.cpp       # General math utility tests
│   │   └── GeometryTests.cpp        # Geometric calculation tests
│   ├── IntegrationTests/
│   │   ├── TransformationTests.cpp  # Combined transformation tests
│   │   ├── ProjectionTests.cpp      # Projection matrix tests
│   │   └── AnimationMathTests.cpp   # Animation math integration tests
│   ├── PerformanceTests/
│   │   ├── SIMDPerformanceTests.cpp # SIMD vs scalar performance
│   │   ├── MatrixPerformanceTests.cpp # Matrix operation benchmarks
│   │   └── VectorPerformanceTests.cpp # Vector operation benchmarks
│   └── Utils/
│       ├── MathTestUtils.hpp        # Test utility functions
│       └── PerformanceTestUtils.hpp # Performance measurement utilities
├── Data/
│   ├── TestVectors.json            # Test vector data sets
│   ├── TestMatrices.json           # Test matrix data sets
│   ├── TestQuaternions.json        # Test quaternion data sets
│   ├── TestConfig.json             # Test configuration settings
│   └── BenchmarkConfig.json        # Performance benchmark settings
└── README.md                       # This documentation
```

## Test Categories

### 1. Unit Tests

**Vector3Tests.cpp**
- Constructor validation (default, parameterized, copy, move)
- Arithmetic operations (+, -, *, /, unary -)
- Compound assignment operators (+=, -=, *=, /=)
- Comparison operations (==, !=)
- Mathematical functions (dot, cross, length, normalize, distance)
- Edge cases (infinity, NaN, zero vectors, very large/small numbers)

**Matrix4Tests.cpp**
- Matrix construction and initialization
- Matrix arithmetic (addition, subtraction, multiplication)
- Matrix operations (transpose, inverse, determinant)
- Transformation matrices (translation, rotation, scaling)
- Projection matrices (perspective, orthographic)
- Matrix-vector transformations

**QuaternionTests.cpp**
- Quaternion construction and normalization
- Quaternion arithmetic and operations
- Axis-angle conversions
- Euler angle conversions
- Spherical linear interpolation (SLERP)
- Quaternion composition and decomposition

### 2. Integration Tests

**TransformationTests.cpp**
- Combined transformation chains
- Matrix-quaternion consistency
- Transform hierarchy validation
- Animation interpolation accuracy

**ProjectionTests.cpp**
- View-projection matrix combinations
- Frustum culling mathematics
- Screen-to-world coordinate transformations
- Depth buffer precision tests

### 3. Performance Tests

**SIMDPerformanceTests.cpp**
- SIMD vs scalar operation comparisons
- Memory alignment impact analysis
- Cache performance evaluation
- Bulk operation throughput measurement

**VectorPerformanceTests.cpp** & **MatrixPerformanceTests.cpp**
- Operation-specific performance benchmarks
- Memory access pattern optimization
- Function call overhead analysis
- Compiler optimization validation

## Running the Tests

### Prerequisites

- Visual Studio 2022 with C++23 support
- Windows 10/11 (x64)
- Google Test framework (included in ThirdParty/)

### Build Configuration

1. **Debug Configuration** (Recommended for development)
   ```bash
   msbuild Tests.Core.Math.vcxproj /p:Configuration=Debug /p:Platform=x64
   ```
   - Enables all assertions and debug checks
   - Includes memory leak detection
   - Provides detailed failure information
   - Runs slower but comprehensive tests

2. **Release Configuration**
   ```bash
   msbuild Tests.Core.Math.vcxproj /p:Configuration=Release /p:Platform=x64
   ```
   - Optimized for speed
   - Skips some slow/exhaustive tests
   - Better for CI/CD pipelines

3. **Profile Configuration** (For performance analysis)
   ```bash
   msbuild Tests.Core.Math.vcxproj /p:Configuration=Profile /p:Platform=x64
   ```
   - Enables performance benchmarks
   - Includes profiling instrumentation
   - Optimized but with debug symbols

### Execution

**From Visual Studio:**
- Set `Tests.Core.Math` as startup project
- Press F5 to run with debugging
- Press Ctrl+F5 to run without debugging

**From Command Line:**
```bash
# Navigate to output directory
cd Build/Output/Bin/x64/Debug/Tests/

# Run all tests
Tests.Core.Math_Debug.exe

# Run specific test suite
Tests.Core.Math_Debug.exe --gtest_filter="Vector3*"

# Run with detailed output
Tests.Core.Math_Debug.exe --gtest_verbose

# Generate XML report
Tests.Core.Math_Debug.exe --gtest_output=xml:results.xml
```

### Test Filtering

Use Google Test filtering to run specific tests:

```bash
# Run only unit tests
--gtest_filter="*UnitTests*"

# Run only performance tests
--gtest_filter="*PerformanceTests*"

# Run only Vector3 tests
--gtest_filter="Vector3*"

# Exclude slow tests
--gtest_filter="-*Stress*:-*Exhaustive*"

# Run specific test case
--gtest_filter="Vector3ArithmeticTests.Addition_WorksCorrectly"
```

## Test Data and Configuration

### Test Data Files

**TestVectors.json**
- Predefined test vectors with expected results
- Edge cases (infinity, NaN, very large/small values)
- Common mathematical constants and special vectors
- Cross-product and dot-product validation data

**TestConfig.json**
- Epsilon tolerance settings for different precision levels
- Performance test iteration counts
- Enable/disable flags for different test categories
- Memory allocation limits and tracking settings

### Configuration Options

Key configuration parameters in `TestConfig.json`:

```json
{
  "core_math_tests": {
    "epsilon_tolerance": 1e-6,           // Standard floating-point comparison
    "loose_epsilon_tolerance": 1e-4,     // For accumulated error scenarios
    "strict_epsilon_tolerance": 1e-8,    // For high-precision requirements
    "enable_performance_tests": true,    // Run performance benchmarks
    "enable_simd_tests": true,          // Test SIMD optimizations
    "performance_iterations": 1000,     // Benchmark iteration count
    "random_seed": 42                   // Reproducible random tests
  }
}
```

## Performance Benchmarking

### SIMD Performance Tests

The test suite includes comprehensive SIMD performance validation:

- **Vector Operations**: Addition, subtraction, multiplication, division
- **Dot/Cross Products**: Optimized vs scalar implementations
- **Length/Normalization**: Fast inverse square root validation
- **Memory Alignment**: Impact of 16-byte vs unaligned data
- **Cache Performance**: Sequential vs random access patterns

### Expected Performance Characteristics

**SIMD Speedup Targets:**
- Vector addition/subtraction: 2-4x speedup over scalar
- Dot products: 1.5-3x speedup
- Normalization: 1.5-2.5x speedup
- Matrix multiplication: 3-8x speedup

**Memory Performance:**
- Aligned access should be ≤10% faster than unaligned
- Sequential access should be 2-5x faster than random
- Cache-friendly algorithms should show clear benefits

## Interpreting Test Results

### Success Criteria

**Unit Tests:**
- All assertions must pass
- No memory leaks detected
- Floating-point comparisons within epsilon tolerance
- Edge cases handled gracefully

**Performance Tests:**
- SIMD implementations should be competitive with or faster than scalar
- No performance regressions > 5% from baseline
- Memory usage within expected bounds
- Cache efficiency metrics within acceptable ranges

### Common Failure Patterns

**Floating-Point Precision Issues:**
```
Expected: 0.333333
Actual: 0.333334
```
- **Cause**: Accumulated floating-point error
- **Solution**: Use appropriate epsilon tolerance or refactor calculation order

**SIMD Performance Regression:**
```
[PERF] SIMD Speedup: 0.8x (expected > 1.0x)
```
- **Cause**: Compiler optimization issues or data alignment problems
- **Solution**: Check compiler flags, data alignment, and SIMD intrinsics usage

**Memory Leaks:**
```
Memory leak detected: 1024 bytes in 16 allocations
```
- **Cause**: Missing cleanup in test fixtures or math library
- **Solution**: Review RAII patterns and ensure proper cleanup

### Debug Output

The test suite provides detailed debug information:

```
========================================
Akhanda Engine - Core.Math Test Suite
========================================

System Information:
  - Compiler: MSVC 1935
  - Architecture: x64
  - SIMD Support: AVX2

[----------] 45 tests from Vector3TestSuite
[ RUNNING  ] Vector3TestSuite.ConstructorTests
[       OK ] Vector3TestSuite.ConstructorTests (1 ms)
[PERF] Vector Addition (SIMD): 12.5 ns/op
[PERF] Vector Addition (Scalar): 23.1 ns/op
[PERF] SIMD Speedup: 1.85x
```

## Maintenance and Extension

### Adding New Tests

1. **Create test file** in appropriate category directory
2. **Include necessary headers** and import math module
3. **Inherit from appropriate fixture** (e.g., `Vector3TestFixture`)
4. **Follow naming conventions**: `TestCategory_SpecificCase_ExpectedBehavior`
5. **Add to project file** and update build dependencies

### Test Naming Conventions

```cpp
// Test Class Pattern
class [Type][Category]Tests : public [Type]TestFixture {};

// Test Method Pattern
TEST_F([TestClass], [Operation]_[Scenario]_[ExpectedResult]) {
    // Test implementation
}

// Examples
TEST_F(Vector3ArithmeticTests, Addition_TwoValidVectors_ReturnsCorrectSum) {}
TEST_F(Matrix4PerformanceTests, Multiplication_LargeMatrices_MeetsPerformanceTarget) {}
```

### Best Practices

1. **Use appropriate epsilon tolerances** for floating-point comparisons
2. **Test edge cases** (zero, infinity, NaN, very large/small values)
3. **Validate both correctness and performance** for critical operations
4. **Use parameterized tests** for testing multiple similar scenarios
5. **Document complex test logic** with clear comments
6. **Verify SIMD correctness** by comparing against scalar implementations

### Performance Baseline Management

The test suite maintains performance baselines for regression detection:

1. **Initial Baseline**: Established during first successful run
2. **Regression Detection**: Automatic comparison against baseline
3. **Baseline Updates**: Manual update when intentional changes are made
4. **CI Integration**: Automated performance validation in build pipeline

## Troubleshooting

### Common Issues

**Build Errors:**
- Ensure C++23 support is enabled
- Verify Google Test libraries are properly linked
- Check module import statements are correct

**Test Failures:**
- Review epsilon tolerance settings
- Check for platform-specific floating-point behavior
- Verify SIMD instruction set availability

**Performance Issues:**
- Ensure Release/Profile configuration for accurate benchmarks
- Verify CPU is not thermally throttling during tests
- Check for background processes affecting performance

### Debug Tools

**Memory Debugging (Debug builds):**
- Automatic leak detection with call stacks
- Allocation tracking and pattern analysis
- Custom allocator validation

**Performance Profiling (Profile builds):**
- Built-in timing instrumentation
- CPU performance counter integration
- Memory access pattern analysis

## Integration with CI/CD

The test suite is designed for automated integration:

```yaml
# Example CI configuration
- name: Build Math Tests
  run: msbuild Tests.Core.Math.vcxproj /p:Configuration=Release

- name: Run Math Tests
  run: Tests.Core.Math_Release.exe --gtest_output=xml:math_results.xml

- name: Validate Performance
  run: Tests.Core.Math_Profile.exe --gtest_filter="*Performance*"
```

**Automated Checks:**
- All unit tests must pass
- No memory leaks detected
- Performance within acceptable thresholds
- No new compiler warnings

## Future Enhancements

Planned improvements for the test suite:

1. **Fuzzing Integration**: Property-based testing for edge case discovery
2. **Cross-Platform Validation**: Linux and macOS test execution
3. **GPU Math Testing**: CUDA/DirectCompute math validation
4. **Precision Analysis**: Numerical stability and error propagation studies
5. **Mutation Testing**: Code quality validation through mutation testing

---

**Version**: 1.0.0  
**Last Updated**: January 2025  
**Maintainer**: Akhanda Engine Team