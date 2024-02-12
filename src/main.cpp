#include <Exceptions.h>
#include <ImageIO.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include <cuda_runtime.h>
#include <FreeImage.h>
#include <cmath>
// #include <npp.h>

#include <helper_cuda.h>

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

void applyBilateralGaussBorder(const std::string &inputFile, const std::string &outputFile)
{
    try
    {
        printf("Starting to process %s \n", inputFile.c_str());
        npp::ImageCPU_8u_C3 hostSrc;
        npp::loadImage(inputFile, hostSrc);
        printf("Loaded image from path \n");
        npp::ImageNPP_8u_C3 deviceSrc(hostSrc);
        const NppiSize srcSize = {(int)deviceSrc.width(), (int)deviceSrc.height()};
        const NppiPoint srcOffset = {0, 0};

        const NppiSize filterROI = {(int)deviceSrc.width(), (int)deviceSrc.height() - 10};
        npp::ImageNPP_8u_C3 deviceDst(filterROI.width, filterROI.height);
        printf("created deviceDst \n");

        // Border
        NPP_CHECK_NPP(nppiFilterBilateralGaussBorder_8u_C3R(deviceSrc.data(), deviceSrc.pitch(), srcSize, srcOffset,
                                                            deviceDst.data(), deviceDst.pitch(), filterROI,
                                                            30, 150, 1000.0, 1000.0, NppiBorderType::NPP_BORDER_REPLICATE));
        printf("Applied with nppiFilterBilateralGaussBorder \n");

        npp::ImageCPU_8u_C3 hostDst(deviceDst.size());
        deviceDst.copyTo(hostDst.data(), hostDst.pitch());
        printf("copied to hostDst \n");
        saveImage(outputFile, hostDst);
        printf("Finished processing %s, writting to %s \n", inputFile.c_str(), outputFile.c_str());

        nppiFree(deviceSrc.data());
        nppiFree(deviceDst.data());
        nppiFree(hostSrc.data());
        nppiFree(hostDst.data());

        cudaDeviceSynchronize();
        cudaDeviceReset();
    }
    catch (npp::Exception &rException)
    {
        std::cerr << "Program error! The following exception occurred: \n";
        std::cerr << rException << std::endl;
        std::cerr << "Aborting." << std::endl;

        exit(EXIT_FAILURE);
    }
    catch (...)
    {
        std::cerr << "Program error! An unknow type of exception occurred. \n";
        std::cerr << "Aborting." << std::endl;

        exit(EXIT_FAILURE);
    }
}

std::tuple<std::string, std::string> parseCommandLineArguments(int argc, char *argv[])
{
    std::string inputPath;
    std::string outputPath;
    for (int i = 1; i < argc; i++)
    {
        std::string option(argv[i]);
        i++;
        std::string value(argv[i]);
        if (option.compare("-i") == 0)
        {
            inputPath = value;
        }
        else if (option.compare("-o") == 0)
        {
            outputPath = value;
        }
    }

    return {inputPath, outputPath};
}

int main(int argc, char *argv[])
{
    printf("%s Starting...\n\n", argv[0]);

    if (!printfNPPinfo(argc, argv))
    {
        exit(EXIT_SUCCESS);
    }

    findCudaDevice(argc, (const char **)argv);

    auto [inputPath, outputPath] = parseCommandLineArguments(argc, argv);

    applyBilateralGaussBorder(inputPath, outputPath);

    return 0;
}
