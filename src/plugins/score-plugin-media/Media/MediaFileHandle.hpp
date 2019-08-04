#pragma once
#include <Media/AudioDecoder.hpp>

#include <score/tools/std/StringHash.hpp>
#include <score/tools/Todo.hpp>

#include <ossia/detail/small_vector.hpp>
#include <ossia/detail/hash_map.hpp>
#include <ossia/audio/drwav_handle.hpp>

#include <QFile>

#include <eggs/variant.hpp>
#include <nano_signal_slot.hpp>
#include <verdigris>

#include <array>
#include <score_plugin_media_export.h>

namespace score
{
struct DocumentContext;
}
namespace Media
{
struct RMSData;
class SoundComponentSetup;
static constexpr float abs_max(float f1, float f2) noexcept
{
  return (std::abs(f1) < std::abs(f2)) ? f2 : f1;
}

struct SCORE_PLUGIN_MEDIA_EXPORT AudioFile final : public QObject
{
public:
  static bool isSupported(const QFile& f);

  AudioFile();
  ~AudioFile() override;

  void load(const QString&, const QString&);

  //! The text passed to the load function
  QString originalFile() const { return m_originalFile; }

  //! Actual filename
  QString fileName() const { return m_fileName; }

  //! Absolute resolved filename
  QString absoluteFileName() const { return m_file; }

  int sampleRate() const { return m_sampleRate; }

  int64_t decodedSamples() const;

  // Number of samples in a channel.
  int64_t samples() const;
  int64_t channels() const;

  bool empty() const { return channels() == 0 || samples() == 0; }

  const RMSData& rms() const { SCORE_ASSERT(m_rms); return *m_rms; }

  Nano::Signal<void()> on_mediaChanged;
  Nano::Signal<void()> on_newData;
  Nano::Signal<void()> on_finishedDecoding;

  void updateSampleRate(int);

  // Note : this is a copy, because it's not thread safe.
  auto handle() const noexcept { return m_impl; }


  struct MmapReader
  {
    std::shared_ptr<QFile> file;
    void* data{};
    ossia::drwav_handle wav;
  };

  struct LibavReader
  {
    LibavReader(int rate) noexcept
      : decoder{rate} { }
    AudioDecoder decoder;
    audio_handle handle;
    ossia::small_vector<audio_sample*, 8> data;
  };

  using libav_ptr = std::shared_ptr<LibavReader>;
  using mmap_ptr = MmapReader;
  using impl_t = eggs::variant<
    mmap_ptr,
    libav_ptr
  >;

  struct Handle : impl_t
  {
    using impl_t::impl_t;
    ossia::small_vector<float, 8> frame(int64_t start_frame) noexcept;
    ossia::small_vector<float, 8> absmax_frame(int64_t start_frame, int64_t end_frame) noexcept;
    ossia::small_vector<std::pair<float, float>, 8> minmax_frame(int64_t start_frame, int64_t end_frame) noexcept;
  };

private:
  void load_ffmpeg(int rate);
  void load_drwav();

  friend class SoundComponentSetup;

  QString m_originalFile;
  QString m_file;
  QString m_fileName;

  RMSData* m_rms{};
  int m_sampleRate{};

  Handle m_impl;
};

class SCORE_PLUGIN_MEDIA_EXPORT AudioFileManager final
    : public QObject
{
public:
  AudioFileManager() noexcept;
  ~AudioFileManager() noexcept;

  static AudioFileManager& instance() noexcept;
  std::shared_ptr<AudioFile> get(const QString&, const score::DocumentContext&);

private:
  ossia::fast_hash_map<QString, std::shared_ptr<AudioFile>> m_handles;
};
}

Q_DECLARE_METATYPE(std::shared_ptr<Media::AudioFile>)
W_REGISTER_ARGTYPE(std::shared_ptr<Media::AudioFile>)
Q_DECLARE_METATYPE(const Media::AudioFile*)
W_REGISTER_ARGTYPE(const Media::AudioFile*)
