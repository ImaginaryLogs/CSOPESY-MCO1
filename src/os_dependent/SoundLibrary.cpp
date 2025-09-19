#include <iostream>
#include <string>


#pragma once


class AudioInterface {
    public:
    
    virtual ~AudioInterface() = default;

    virtual bool load(const std::string& file) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
};


#if defined(_WIN32)


#include <windows.h>
#include <mmsystem.h>

class WinMMSoundPlayer : public AudioInterface {
  std::string currentFile;
  bool loaded = false;
  bool paused = false;

public:
  bool load(const std::string& file) override {
      currentFile = file;

      // Close any previous media
      mciSendString("close mySound", NULL, 0, NULL);

      // Open new file
      std::string cmd = "open \"" + file + "\" type mpegvideo alias mySound";
      if (mciSendString(cmd.c_str(), NULL, 0, NULL) != 0) {
          return false;
      }

      loaded = true;
      paused = false;
      return true;
  }

  void play() override {
      if (!loaded) return;

      if (paused) {
          // Resume from pause
          mciSendString("resume mySound", NULL, 0, NULL);
          paused = false;
      } else {
          // Play from start
          mciSendString("play mySound", NULL, 0, NULL);
      }
  }

  void pause() override {
      if (!loaded) return;
      mciSendString("pause mySound", NULL, 0, NULL);
      paused = true;
  }

  void stop() override {
      if (!loaded) return;
      mciSendString("stop mySound", NULL, 0, NULL);
      mciSendString("seek mySound to start", NULL, 0, NULL);
      paused = false;
  }

  ~WinMMSoundPlayer() {
      if (loaded) {
          mciSendString("close mySound", NULL, 0, NULL);
      }
  }
};

#else 

#include <SFML/Audio.hpp>

class SFMLSoundPlayer : public AudioInterface {
  private:
  sf::Music music;
  
  public:
  
  bool load(const std::string& file) override {
      if (!music.openFromFile(file)) {
          std::cerr << "Failed to load " << file << "\n";
          return false;
      }
      return true;
  }

  void play() override {
      music.play();
  }

  void stop() override {
      music.stop();
  }

  void pause() override {
      music.pause();
  }
};


#endif





