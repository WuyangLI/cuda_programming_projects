#include <Exceptions.h>
#include <ImageIO.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include <nppi.h>
#include <nppdefs.h>


bool printfNPPinfo(int argc, char *argv[])
{
    const NppLibraryVersion *libVer = nppGetLibVersion();

    printf("NPP Library Version %d.%d.%d\n", libVer->major, libVer->minor,
           libVer->build);

    int driverVersion, runtimeVersion;
    cudaDriverGetVersion(&driverVersion);
    cudaRuntimeGetVersion(&runtimeVersion);

    printf("  CUDA Driver  Version: %d.%d\n", driverVersion / 1000,
           (driverVersion % 100) / 10);
    printf("  CUDA Runtime Version: %d.%d\n", runtimeVersion / 1000,
           (runtimeVersion % 100) / 10);

    // Min spec is SM 1.0 devices
    bool bVal = checkCudaCapabilities(1, 0);
    return bVal;
}

bool loadImage(const std::string& imagePath, Npp8u*& imageBuffer, int& width, int& height) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open image file: " << imagePath << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    Npp8u* buffer = new Npp8u[fileSize];
    file.read(reinterpret_cast<char*>(buffer), fileSize);
    file.close();

    NppiSize size;
    Npp8u* pImage = nppiDecodePNGGetInfo(buffer, static_cast<int>(fileSize), &size);

    if (pImage == nullptr) {
        std::cerr << "Failed to decode PNG file: " << imagePath << std::endl;
        delete[] buffer;
        return false;
    }

    width = size.width;
    height = size.height;
    imageBuffer = new Npp8u[size.width * size.height];
    nppiDecodePNGHost(pImage, static_cast<int>(fileSize), imageBuffer, size);

    delete[] buffer;
    return true;
}

bool saveImage(const std::string& outputPath, const Npp8u* imageBuffer, int width, int height) {
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create output image file: " << outputPath << std::endl;
        return false;
    }

    NppiSize size = {width, height};
    Npp8u* pPngData;
    int pngSize;
    if (nppiEncodePNGHost(imageBuffer, size, &pPngData, &pngSize) != NPP_SUCCESS) {
        std::cerr << "Failed to encode image data to PNG format." << std::endl;
        return false;
    }

    file.write(reinterpret_cast<char*>(pPngData), pngSize);
    file.close();
    return true;
}


Npp8u* edge_mask(const Npp8u* img, int width, int height) {
    int block_size = 17; 
    int edge_blur_val = 9;
    // Convert image to grayscale
    NppiSize imageSize = {width, height};
    Npp8u* d_gray;
    size_t grayPitch;
    nppiMalloc_8u_C1(width, height, &d_gray, &grayPitch);
    nppiRGBToGray_8u_C3C1R(img, width * 3, d_gray, grayPitch, imageSize);

    // Apply Gaussian blur
    Npp8u* d_gray_blur;
    size_t grayBlurPitch;
    nppiMalloc_8u_C1(width, height, &d_gray_blur, &grayBlurPitch);
    NppiSize blurSize = {edge_blur_val, edge_blur_val};
    NppiSize roiSize = {width - edge_blur_val + 1, height - edge_blur_val + 1};
    nppiFilterGauss_8u_C1R(d_gray, grayPitch, d_gray_blur, grayBlurPitch, roiSize, blurSize, {0, 0});

    // Apply adaptive thresholding
    Npp8u* d_edges;
    size_t edgesPitch;
    nppiMalloc_8u_C1(width, height, &d_edges, &edgesPitch);
    nppiThreshold_Adv_8u_C1R(d_gray_blur, grayBlurPitch, d_edges, edgesPitch, imageSize, 255, NPP_THRESH_BINARY, block_size, 2);

    // Free memory
    nppiFree(d_gray);
    nppiFree(d_gray_blur);

    return d_edges;
}

void quantizeImage(const Npp8u* inputImage, int width, int height, Npp8u* outputImage) {
    // Divide each pixel value by 50
    nppiDivC_8u_C1IR(inputImage, width, 50, outputImage, width, {width, height});

    // Multiply each pixel value by 50
    nppiMulC_8u_C1IR(outputImage, width, 50, outputImage, width, {width, height});
}

void gaussianBlur(const Npp8u* img, Npp8u* blurred, int width, int height, int colorBlurVal) {
    NppiSize imageSize = {width, height};
    NppiSize kernelSize = {colorBlurVal, colorBlurVal};
    NppiPoint anchor = {colorBlurVal / 2, colorBlurVal / 2}; // Center of the kernel

    // Apply Gaussian blur to the input image
    nppiFilterGaussBorder_8u_C3R(img, width * 3, blurred, width * 3, imageSize, kernelSize, anchor);
}

void bitwiseAndWithMask(const Npp8u* recolored, const Npp8u* edges, Npp8u* output, int width, int height) {
    // Perform the bitwise AND operation between recolored and itself with edges as a mask
    NppiSize roiSize = {width, height};
    nppiAnd_8u_C3MR(recolored, width * 3, recolored, width * 3, output, width * 3, roiSize, edges, width, height);
}

void applyFilter(const std::string &inputPath, const std::string &outputPath)
{
    Npp8u* imageBuffer;
    int width, height;

    // Load the input image
    if (!loadImage(inputImagePath, imageBuffer, width, height)) {
        return EXIT_FAILURE;
    }

    // 
    Npp8u* edges = edge_mask(imageBuffer, width, height);
    // cudaDeviceSynchronize();

    Npp8u* blurred = new Npp8u[width * height];
    int colorBlurVal = 27;
    gaussianBlur(imageBuffer, blurred, width, height, colorBlurVal);
    // cudaDeviceSynchronize();

    Npp8u* quantized = new Npp8u[width * height];
    quantizeImage(blurred, width, height, quantized);
    // cudaDeviceSynchronize();

    Npp8u* output = new Npp8u[width * height];
    bitwiseAndWithMask(quantized, edges, output, width, height);
    cudaDeviceSynchronize();

    // 
    cudaDeviceReset();
    // Save the input image as PNG
    saveImage(outputImagePath, output, width, height);

    std::cout << "Image saved successfully to: " << outputImagePath << std::endl;
    delete[] imageBuffer;
    delete[] edges;
    delete[] blurred;
    delete[] quantized;
    delete[] output;
}


std::tuple<int, std::string, int, std::string> parseCommandLineArguments(int argc, char *argv[])
{
    int threadsPerBlock = 256;

    for(int i = 1; i < argc; i++)
    {
        std::string option(argv[i]);
        i++;
        std::string value(argv[i]);
        if(option.compare("-i") == 0) 
        {
            inputPath = value;
        }
        else if(option.compare("-o") == 0) 
        {
            outputPath = value;
        }
    }

    return {inputPath, outputPath, threadsPerBlock};
}

int main(int argc, char *argv[])
{
    printf("%s Starting...\n\n", argv[0]);

    if (!printfNPPinfo(argc, argv))
    {
        exit(EXIT_SUCCESS);
    }

    findCudaDevice(argc, (const char **)argv);

    auto[inputPath, outputPath, threadsPerBlock] = parseCommandLineArguments(argc, argv);

    applyFilter(inputPath, outputPath);

    return 0;
}
