#include "../os_dependent/SoundLibrary.cpp"
#include <iostream>
#include "Context.cpp"
class AudioHandler :

#if defined(_WIN32)
    public WinMMSoundPlayer
#else 
    public SFMLSoundPlayer
#endif
    , public Handler
{
 
 public:
    AudioHandler(MarqueeContext& c) : Handler(c) {}

    void operator()(){

    }

    void ping() {
        std::cout << "Audio handler connected." << std::endl;
    }
};
