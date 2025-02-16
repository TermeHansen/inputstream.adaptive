/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SubtitleSampleReader.h"

#include "../utils/log.h"

#include <kodi/Filesystem.h>

using namespace SESSION;

CSubtitleSampleReader::CSubtitleSampleReader(const std::string& url,
                                           AP4_UI32 streamId,
                                           const std::string& codecInternalName)
  : m_streamId{streamId}
{
  // open the file
  kodi::vfs::CFile file;
  if (!file.CURLCreate(url))
    return;

  file.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL, "seekable", "0");
  file.CURLAddOption(ADDON_CURL_OPTION_PROTOCOL, "acceptencoding", "gzip");
  file.CURLOpen(ADDON_READ_CHUNKED | ADDON_READ_NO_CACHE);

  AP4_DataBuffer result;

  // read the file
  static const unsigned int CHUNKSIZE = 16384;
  AP4_Byte buf[CHUNKSIZE];
  size_t nbRead;
  while ((nbRead = file.Read(buf, CHUNKSIZE)) > 0 && ~nbRead)
    result.AppendData(buf, nbRead);
  file.Close();

  if (codecInternalName == "wvtt")
    m_codecHandler = new WebVTTCodecHandler(nullptr, true);
  else
    m_codecHandler = new TTMLCodecHandler(nullptr);

  m_codecHandler->Transform(0, 0, result, 1000);
}

CSubtitleSampleReader::CSubtitleSampleReader(CStream* stream,
                                           AP4_UI32 streamId,
                                           const std::string& codecInternalName)
  : m_streamId{streamId}, m_adByteStream{stream->GetAdByteStream()}, m_adStream{&stream->m_adStream}
{
  if (codecInternalName == "wvtt")
    m_codecHandler = new WebVTTCodecHandler(nullptr, false);
  else
    m_codecHandler = new TTMLCodecHandler(nullptr);
}

AP4_Result CSubtitleSampleReader::Start(bool& bStarted)
{
  m_eos = false;
  return AP4_SUCCESS;
}

AP4_Result CSubtitleSampleReader::ReadSample()
{
  if (m_codecHandler->ReadNextSample(m_sample,
                                     m_sampleData)) // Read the sample data from a file url
  {
    m_pts = m_sample.GetCts() * 1000;
    return AP4_SUCCESS;
  }
  else if (m_adByteStream) // Read the sample data from a segment file stream (e.g. HLS)
  {
    // Get the next segment
    if (m_adStream && m_adStream->ensureSegment())
    {
      size_t segSize;
      if (m_adStream->retrieveCurrentSegmentBufferSize(segSize))
      {
        AP4_DataBuffer segData;
        while (segSize > 0)
        {
          AP4_Size readSize = m_segmentChunkSize;
          if (segSize < static_cast<size_t>(m_segmentChunkSize))
            readSize = static_cast<AP4_Size>(segSize);

          AP4_Byte* buf = new AP4_Byte[readSize];
          segSize -= readSize;
          if (AP4_SUCCEEDED(m_adByteStream->Read(buf, readSize)))
          {
            segData.AppendData(buf, readSize);
            delete[] buf;
          }
          else
          {
            delete[] buf;
            break;
          }
        }
        auto rep = m_adStream->getRepresentation();
        if (rep)
        {
          auto currentSegment = rep->current_segment_;
          if (currentSegment)
          {
            m_codecHandler->Transform(currentSegment->startPTS_, currentSegment->m_duration,
                                      segData, 1000);
            if (m_codecHandler->ReadNextSample(m_sample, m_sampleData))
            {
              m_pts = m_sample.GetCts();
              m_ptsDiff = m_pts - m_ptsOffset;
              return AP4_SUCCESS;
            }
          }
          else
            LOG::LogF(LOGERROR, "Failed to get current segment of subtitle stream");
        }
        else
          LOG::LogF(LOGERROR, "Failed to get Representation of subtitle stream");
      }
      else
      {
        LOG::LogF(LOGERROR, "Failed to get subtitle segment buffer size");
      }
    }
  }
  m_eos = true;
  return AP4_ERROR_EOS;
}

void CSubtitleSampleReader::Reset(bool bEOS)
{
  if (m_adByteStream || bEOS)
    m_codecHandler->Reset();
}

bool CSubtitleSampleReader::GetInformation(kodi::addon::InputstreamInfo& info)
{
  if (m_codecHandler->m_extraData.GetDataSize() &&
      !info.CompareExtraData(m_codecHandler->m_extraData.GetData(),
                             m_codecHandler->m_extraData.GetDataSize()))
  {
    info.SetExtraData(m_codecHandler->m_extraData.GetData(),
                      m_codecHandler->m_extraData.GetDataSize());
    return true;
  }
  return false;
}

bool CSubtitleSampleReader::TimeSeek(uint64_t pts, bool preceeding)
{
  if (dynamic_cast<WebVTTCodecHandler*>(m_codecHandler))
  {
    m_pts = pts;
    return true;
  }
  else
  {
    if (m_codecHandler->TimeSeek(pts / 1000))
      return AP4_SUCCEEDED(ReadSample());
    return false;
  }
}
