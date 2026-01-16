#include "Sound/sound.h"

Sound GESound::intro{};
Sound GESound::soundButtonHover1{};
Sound GESound::soundButtonHover2{};
Sound GESound::soundButtonHover3{};
Sound GESound::soundButtonEnable{};
Sound GESound::soundButtonDisable{};
Sound GESound::soundSliderSlide{};

std::vector<Sound> GESound::soundButtonHover1Pool{};
std::vector<Sound> GESound::soundButtonHover2Pool{};
std::vector<Sound> GESound::soundButtonHover3Pool{};
std::vector<Sound> GESound::soundButtonEnablePool{};
std::vector<Sound> GESound::soundButtonDisablePool{};
std::vector<Sound> GESound::soundSliderSlidePool{};

void GESound::loadSounds() {}

void GESound::soundtrackLogic() {}

void GESound::unloadSounds() {}
