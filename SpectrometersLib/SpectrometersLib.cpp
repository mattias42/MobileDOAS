// SpectrometersLib.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include <memory>
#include <vector>
#include <MobileDoasLib/Measurement/SpectrometerInterface.h>
#include "Avantes/AvantesSpectrometerInterface.h"
#include "OceanOptics/OceanOpticsSpectrometerInterface.h"

namespace mobiledoas {

    std::vector<std::unique_ptr<mobiledoas::SpectrometerInterface>> ListSpectrometerInterfaces() {

        std::vector<std::unique_ptr<mobiledoas::SpectrometerInterface>> result;

#ifdef MANUFACTURER_SUPPORT_OCEANOPTICS
        result.push_back(std::make_unique<oceanoptics::OceanOpticsSpectrometerInterface>());
#endif // MANUFACTURER_SUPPORT_OCEANOPTICS

#ifdef MANUFACTURER_SUPPORT_AVANTES
        result.push_back(std::make_unique<avantes::AvantesSpectrometerInterface>());
#endif // MANUFACTURER_SUPPORT_AVANTES

        return result;
    }

}