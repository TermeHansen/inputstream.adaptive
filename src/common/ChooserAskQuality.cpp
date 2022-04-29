/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ChooserAskQuality.h"

#include "../utils/StringUtils.h"
#include "../utils/Utils.h"
#include "../utils/log.h"
#include "ReprSelector.h"
#include "kodi/tools/StringUtils.h"

#ifndef INPUTSTREAM_TEST_BUILD
#include <kodi/gui/dialogs/Select.h>
#endif

#include <vector>

using namespace adaptive;
using namespace CHOOSER;
using namespace UTILS;

CRepresentationChooserAskQuality::CRepresentationChooserAskQuality()
{
  LOG::Log(LOGDEBUG, "[Repr. chooser] Type: Ask quality");
}

void CRepresentationChooserAskQuality::Initialize(const UTILS::PROPERTIES::ChooserProps& props)
{
}

void CRepresentationChooserAskQuality::PostInit()
{
}

AdaptiveTree::Representation* CRepresentationChooserAskQuality::ChooseRepresentation(
    AdaptiveTree::AdaptationSet* adp)
{
  if (adp->type_ != AdaptiveTree::VIDEO)
  {
    CRepresentationSelector selector{m_screenCurrentWidth, m_screenCurrentHeight};
    return selector.HighestBw(adp);
  }

  //! @todo: m_isFirstVideoAdaptationSetChosen is a kind of workaround,
  //! currently we dont handle in any way a codec priority and selection
  //! that can happens when a manifest have multi-codec videos, therefore
  //! we sent to Kodi the video stream of each codec, but only the
  //! first one (in index order) will be choosen for the playback
  //! with the potential to poorly manage a bandwidth optimisation.
  //! So we ask to the user to select the quality only for the first video
  //! AdaptationSet and we try to select the same quality (resolution)
  //! on all other video AdaptationSet's (codecs)
  if (!m_isFirstVideoAdaptationSetChosen)
  {
    // We find the best quality for the current resolution, to pre-select this entry
    CRepresentationSelector selector{m_screenCurrentWidth, m_screenCurrentHeight};
    AdaptiveTree::Representation* bestRep{selector.Highest(adp)};

    std::vector<std::string> entries;
    int preselIndex{-1};

    // Add available qualities
    for (size_t i{0}; i < adp->representations_.size(); i++)
    {
      AdaptiveTree::Representation* rep{adp->representations_[i]};

      std::string entryName{kodi::addon::GetLocalizedString(30232)};
      STRING::ReplaceFirst(entryName, "{codec}", GetVideoCodecDesc(rep->codecs_));
      STRING::ReplaceFirst(entryName, "{quality}",
                           kodi::tools::StringUtils::Format("(%ix%i, %u Kbps)", rep->width_,
                                                            rep->height_, rep->bandwidth_ / 1000));

      if (rep == bestRep)
        preselIndex = static_cast<int>(i);

      entries.emplace_back(entryName);
    }

    int selIndex{kodi::gui::dialogs::Select::Show(kodi::addon::GetLocalizedString(30231), entries,
                                                  preselIndex, 10000)};

    AdaptiveTree::Representation* selRep{bestRep};

    if (selIndex >= 0) // If was <0 has been cancelled by the user
      selRep = adp->representations_[selIndex];

    m_selectedResWidth = selRep->width_;
    m_selectedResHeight = selRep->height_;
    m_isFirstVideoAdaptationSetChosen = true;

    return selRep;
  }
  else
  {
    // The user has already chosen the quality, we try select the same resolution
    // in any case these are choosable only manually via Kodi OSD video settings.
    CRepresentationSelector selector{m_selectedResWidth, m_selectedResHeight};
    return selector.Highest(adp);
  }
}

AdaptiveTree::Representation* CRepresentationChooserAskQuality::ChooseNextRepresentation(
    AdaptiveTree::AdaptationSet* adp, AdaptiveTree::Representation* currentRep)
{
  return currentRep;
}