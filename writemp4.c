#include <stdio.h>
#include <stdlib.h>
#include <gpac/isomedia.h>
#include <gpac/internal/isomedia_dev.h>
#include <gpac/media_tools.h>
#include <gpac/constants.h>

#include "def.h"
#include "bits.h"
#include "misc.h"
#include "writemp4.h"

static void write_sample(GF_ISOSample *smp, GF_AVCConfig *avccfg, GF_M4ADecSpecInfo *acfg, u32 is_aac, u32 aac_type, GF_BitStream *bs)
{
  if (!avccfg) {
    if (is_aac) {
      gf_bs_write_int(bs, 0xFFF, 12);
      gf_bs_write_int(bs, is_aac == 1 ? 1 : 0, 1);
      gf_bs_write_int(bs, 0, 2);
      gf_bs_write_int(bs, 1, 1);
      gf_bs_write_int(bs, aac_type, 2);
      gf_bs_write_int(bs, acfg->base_sr_index, 4);
      gf_bs_write_int(bs, 0, 1);
      gf_bs_write_int(bs, acfg->nb_chan, 3);
      gf_bs_write_int(bs, 0, 4);
      gf_bs_write_int(bs, 7 + smp->dataLength, 13);
      gf_bs_write_int(bs, 0x7FF, 11);
      gf_bs_write_int(bs, 0, 2);
    }
    gf_bs_write_data(bs, smp->data, smp->dataLength);
  } else {
    u32 j, nal_size, remain = smp->dataLength;
    u8 *ptr = smp->data;
    while (remain) {
      nal_size = 0;
      for (j = 0; j < avccfg->nal_unit_size; j++) {
        nal_size |= *ptr;
        if (j + 1 < avccfg->nal_unit_size) nal_size <<= 8;
        remain--;
        ptr++;
      }
      gf_bs_write_u32(bs, 1);
      if (nal_size > smp->dataLength || nal_size == 0) {
        gf_bs_write_data(bs, ptr, remain);
        remain = 0;
      } else {
        gf_bs_write_data(bs, ptr, nal_size);
        ptr += nal_size;
        remain -= nal_size;
      }
    }
  }
}

static int write_raw(GF_ISOFile *fi, u32 hint, u32 data, char *fn, data_t *D, MODE mode)
{
  GF_DecoderConfig *dcfg = 0;
  GF_M4VDecSpecInfo dsi = { 0 };
  GF_M4ADecSpecInfo acfg = { 0 };
  GF_AVCConfig *avccfg = 0;
  GF_BitStream *bs, *bs_tmp;
  GF_ISOSample *smp = 0, *tmp = 0;
  GF_HintSample *hint_smp;
  GF_RTPPacket *pck;
  FILE *fo;

  char *pdsi = 0;
  u32 i, j, k, l, h, is_aac = 0, aac_type = 0;
  u32 di, count, packets, samples, last_smp = 0, m_stype, dsi_size = 0;

  m_stype = gf_isom_get_media_subtype(fi, data, 1);

  if (m_stype == GF_ISOM_SUBTYPE_MPEG4 || m_stype == GF_ISOM_SUBTYPE_MPEG4_CRYP)
    dcfg = gf_isom_get_decoder_config(fi, data, 1);

  if (dcfg) {
    switch (dcfg->streamType) {
    case GF_STREAM_VISUAL:
      switch (dcfg->objectTypeIndication) {
        case 0x20:
          pdsi = dcfg->decoderSpecificInfo->data;
          dsi_size = dcfg->decoderSpecificInfo->dataLength;
          gf_m4v_get_config(pdsi, dsi_size, &dsi);
          break;
        case 0x21:
          avccfg = gf_isom_avc_config_get(fi, data, 1);
          break;
        default:
          gf_odf_desc_del((GF_Descriptor *) dcfg);
          return 0;
      }
      break;
    case GF_STREAM_AUDIO:
			switch (dcfg->objectTypeIndication) {
			  case 0x66:
			  case 0x67:
			  case 0x68:
				  pdsi = dcfg->decoderSpecificInfo->data;
				  dcfg->decoderSpecificInfo->data = NULL;
				  dsi_size = dcfg->decoderSpecificInfo->dataLength;
				  is_aac = 1;
				  aac_type = dcfg->objectTypeIndication - 0x66;
				  break;
			  case 0x40:
				  pdsi = dcfg->decoderSpecificInfo->data;
				  dcfg->decoderSpecificInfo->data = NULL;
				  dsi_size = dcfg->decoderSpecificInfo->dataLength;
				  is_aac = 2;
				  break;
			  case 0x69:
			  case 0x6B:
				  break;
        default:
          gf_odf_desc_del((GF_Descriptor *) dcfg);
          return 0;
      }
      break;
    default:
      gf_odf_desc_del((GF_Descriptor *) dcfg);
      return 0;
    }
  } else {
    switch (m_stype) {
      case GF_ISOM_SUBTYPE_AVC_H264:
        avccfg = gf_isom_avc_config_get(fi, data, 1);
        break;
      case GF_ISOM_SUBTYPE_3GP_H263: break;
      default: return 0;
    }
  }
  if (0 == (fo = fopen(fn, "wb"))) goto O;
  bs = gf_bs_from_file(fo, GF_BITSTREAM_WRITE);

  if (is_aac) {
		gf_m4a_get_config(pdsi, dsi_size, &acfg);
		if (is_aac == 2) aac_type = acfg.base_object_type - 1;
		free(pdsi);
		pdsi = 0;
	}
	if (pdsi) {
		gf_bs_write_data(bs, pdsi, dsi_size);
		free(pdsi);
	}
  if (dcfg) gf_odf_desc_del((GF_Descriptor *)dcfg);

  if (avccfg) {
    count = gf_list_count(avccfg->sequenceParameterSets);
    for (i = 0; i < count; i++) {
      GF_AVCConfigSlot *sl = gf_list_get(avccfg->sequenceParameterSets, i);
      gf_bs_write_u32(bs, 1);
      gf_bs_write_data(bs, sl->data, sl->size);
    }
    count = gf_list_count(avccfg->pictureParameterSets);
    for (i = 0; i < count; i++) {
      GF_AVCConfigSlot *sl = gf_list_get(avccfg->pictureParameterSets, i);
      gf_bs_write_u32(bs, 1);
      gf_bs_write_data(bs, sl->data, sl->size);
    }
  }
  samples = gf_isom_get_sample_count(fi, !(mode & AUDIO) && (mode & FRAME || mode & COMPLETE) ? data : hint);
  for (l = 0, i = 0; i < samples; i++) {
    if (0 == (smp = gf_isom_get_sample(fi, !(mode & AUDIO) && (mode & FRAME || mode & COMPLETE) ? data : hint, i + 1, &di))) goto S;
    if (mode & AUDIO) {
      if (!D->F[i].lost) {
        hint_smp = gf_isom_hint_sample_new(GF_ISOM_BOX_TYPE_RTP_STSD);
        bs_tmp = gf_bs_new(smp->data, smp->dataLength, GF_BITSTREAM_READ);
        gf_isom_hint_sample_read(hint_smp, bs_tmp, smp->dataLength);
        gf_bs_del(bs_tmp);
        packets = gf_list_count(hint_smp->packetTable);
        for (j = 0; j < packets; j++, l++) {
          pck = gf_list_get(hint_smp->packetTable, j);
          count = gf_list_count(pck->DataTable);
          for (k = 0; k < count; k++) {
            GF_GenericDTE *dte = gf_list_get(pck->DataTable, k);
            if (dte->source == 2) {
              GF_SampleDTE *sdte = (GF_SampleDTE *)dte;
              if (sdte->sampleNumber != last_smp) {
                if (last_smp != 0) {
                  write_sample(tmp, avccfg, &acfg, is_aac, aac_type, bs);
                  gf_isom_sample_del(&tmp);
                }
                if (0 == (tmp = gf_isom_get_sample(fi, data, sdte->sampleNumber, 0))) goto S;
                last_smp = sdte->sampleNumber;
              }
            }
          }
        }
      }
    }
    else if (mode & FRAME || mode & COMPLETE) {
      if (D->F[i].lost) {
        h = 0;
        if (m_stype == 'MPEG') {
          if (!dsi.NumBitsTimeIncrement ||
            !(h = mark_not_coded(smp->data, smp->dataLength, dsi.NumBitsTimeIncrement)))
            goto E;
        }
        if (m_stype == 'avc1' && i == 0) h = 64;
        if (smp->dataLength > 8 + h) {
          if (mode & TRUNC) smp->dataLength = 8 + h;
          if (mode & FILL) memset(smp->data + 8 + h, 0, smp->dataLength - (8 + h));
        }
      }
      write_sample(smp, avccfg, 0, 0, 0, bs);
      gf_isom_sample_del(&smp);
    } else {
      hint_smp = gf_isom_hint_sample_new(GF_ISOM_BOX_TYPE_RTP_STSD);
      bs_tmp = gf_bs_new(smp->data, smp->dataLength, GF_BITSTREAM_READ);
      gf_isom_hint_sample_read(hint_smp, bs_tmp, smp->dataLength);
      gf_bs_del(bs_tmp);
      packets = gf_list_count(hint_smp->packetTable);
      for (j = 0; j < packets; j++, l++) {
        pck = gf_list_get(hint_smp->packetTable, j);
        count = gf_list_count(pck->DataTable);
        for (k = 0; k < count; k++) {
          GF_GenericDTE *dte = gf_list_get(pck->DataTable, k);
          if (dte->source == 2) {
            GF_SampleDTE *sdte = (GF_SampleDTE *)dte;
            if (sdte->sampleNumber != last_smp) {
              if (last_smp != 0) {
                if (!D->P[l].lost) write_sample(tmp, avccfg, 0, 0, 0, bs);
                gf_isom_sample_del(&tmp);
              }
              if (0 == (tmp = gf_isom_get_sample(fi, data, sdte->sampleNumber, 0))) goto S;
              last_smp = sdte->sampleNumber;
            }
            if (D->P[l].lost && !(l == 0 && m_stype == 'avc1')) {
              if (mode & FILL && sdte->dataLength > 8) {
                memset(tmp->data + sdte->byteOffset + 8, 0, sdte->dataLength - 8);
              }
            }
          }
        }
      }
    }
  }
  if (tmp) write_sample(tmp, avccfg, &acfg, is_aac, aac_type, bs);
  gf_isom_sample_del(&tmp);
  if (avccfg) gf_odf_avc_cfg_del(avccfg);
  gf_bs_del(bs);
  fclose(fo);

  return 1;

E: fprintf(stderr, "Error parsing MPEG-4 VOP header\n"); goto X;
O: fprintf(stderr, "Error opening %s for writing\n", fn); goto X;
S: fprintf(stderr, "Couldn't retrieve sample %d from track %d\n", i + 1, data);

X: if (dcfg) gf_odf_desc_del((GF_Descriptor *)dcfg);
   if (avccfg) gf_odf_avc_cfg_del(avccfg);
   return 0;
}

static int write_mp4(GF_ISOFile *fi, u32 hint, u32 data, char *fn, data_t *D, MODE mode)
{
  GF_ESD *esd = 0;
  GF_ISOSample *smp = 0, *tmp = 0;
  GF_HintSample *hint_smp;
  GF_InitialObjectDescriptor *iod = 0;
  GF_M4VDecSpecInfo dsi = {0};
  GF_RTPPacket *pck;
  GF_BitStream *bs_tmp;
  GF_ISOFile *fo;

  u32 i, j, k, l, count, ts, rate, dur, pos, di, samples, packets, last_smp = 0;
  u32 h, newTk, descIndex, msubtype, trackID = gf_isom_get_track_id(fi, data);

  if (0 == (fo = gf_isom_open(fn, GF_ISOM_WRITE_EDIT, 0))) goto O;
  if (GF_ISOM_SUBTYPE_MPEG4 == (msubtype = gf_isom_get_media_subtype(fi, data, 1))) {
    if (0 != (esd = gf_isom_get_esd(fi, data, 1))) {
      esd->dependsOnESID = 0;
      esd->OCRESID = 0;
    }
  }
  newTk = gf_isom_new_track(fo, trackID, gf_isom_get_media_type(fi, data), gf_isom_get_media_timescale(fi, data));
  gf_isom_set_track_enabled(fo, newTk, 1);
  if (esd) {
    gf_isom_new_mpeg4_description(fo, newTk, esd, 0, 0, &descIndex);
    if ((esd->decoderConfig->streamType == GF_STREAM_VISUAL) || (esd->decoderConfig->streamType == GF_STREAM_SCENE)) {
      u32 w, h;
      gf_isom_get_visual_info(fi, data, 1, &w, &h);
      if ((esd->decoderConfig->objectTypeIndication == 32)) {
        gf_m4v_get_config(esd->decoderConfig->decoderSpecificInfo->data, esd->decoderConfig->decoderSpecificInfo->dataLength, &dsi);
        w = dsi.width;
        h = dsi.height;
      }
      gf_isom_set_visual_info(fo, newTk, 1, w, h);
    }
    esd->decoderConfig->avgBitrate = 0;
    esd->decoderConfig->maxBitrate = 0;
  } else {
    gf_isom_clone_sample_description(fo, newTk, fi, data, 1, 0, 0, &descIndex);
  }
  pos = rate = 0;
  ts = gf_isom_get_media_timescale(fi, data);
  samples = gf_isom_get_sample_count(fi, !(mode & AUDIO) && (mode & FRAME || mode & COMPLETE) ? data : hint); 
  for (l = 0, i = 0; i < samples; i++) {
    if (0 == (smp = gf_isom_get_sample(fi, !(mode & AUDIO) && (mode & FRAME || mode & COMPLETE) ? data : hint, i + 1, &di))) goto R;
    if (mode & AUDIO) {
      if (!D->F[i].lost) {
        hint_smp = gf_isom_hint_sample_new(GF_ISOM_BOX_TYPE_RTP_STSD);
        bs_tmp = gf_bs_new(smp->data, smp->dataLength, GF_BITSTREAM_READ);
        gf_isom_hint_sample_read(hint_smp, bs_tmp, smp->dataLength);
        gf_bs_del(bs_tmp);
        packets = gf_list_count(hint_smp->packetTable);
        for (j = 0; j < packets; j++, l++) {
          pck = gf_list_get(hint_smp->packetTable, j);
          count = gf_list_count(pck->DataTable);
          for (k = 0; k < count; k++) {
            GF_GenericDTE *dte = gf_list_get(pck->DataTable, k);
            if (dte->source == 2) {
              GF_SampleDTE *sdte = (GF_SampleDTE *)dte;
              if (sdte->sampleNumber != last_smp) {
                if (last_smp != 0) {
                  if (GF_OK != gf_isom_add_sample(fo, newTk, descIndex, tmp)) goto A;
                  gf_isom_sample_del(&tmp);
                }
                if (0 == (tmp = gf_isom_get_sample(fi, data, sdte->sampleNumber, 0))) goto R;
                last_smp = sdte->sampleNumber;
              }
            }
          }
        }
      }
    }
    else if (mode & FRAME || mode & COMPLETE) {
      if (D->F[i].lost) {
        h = 0;
        if (msubtype == 'MPEG') {
          if (!dsi.NumBitsTimeIncrement ||
              !(h = mark_not_coded(smp->data, smp->dataLength, dsi.NumBitsTimeIncrement)))
            goto E;
        }
        if (msubtype == 'avc1' && l == 0) h = 64;
        if (smp->dataLength > 8 + h) {
          if (mode & TRUNC) smp->dataLength = 8 + h;
          if (mode & FILL) memset(smp->data + 8 + h, 0, smp->dataLength - (8 + h));
        }
      }
      if (GF_OK != gf_isom_add_sample(fo, newTk, descIndex, smp)) goto A;
    } else {
      hint_smp = gf_isom_hint_sample_new(GF_ISOM_BOX_TYPE_RTP_STSD);
      bs_tmp = gf_bs_new(smp->data, smp->dataLength, GF_BITSTREAM_READ);
      gf_isom_hint_sample_read(hint_smp, bs_tmp, smp->dataLength);
      gf_bs_del(bs_tmp);
      packets = gf_list_count(hint_smp->packetTable);
      for (j = 0; j < packets; j++, l++) {
        pck = gf_list_get(hint_smp->packetTable, j);
        count = gf_list_count(pck->DataTable);
        for (k = 0; k < count; k++) {
          GF_GenericDTE *dte = gf_list_get(pck->DataTable, k);
          if (dte->source == 2) {
            GF_SampleDTE *sdte = (GF_SampleDTE *)dte;
            if (sdte->sampleNumber != last_smp) {
              if (last_smp != 0) {
                if (GF_OK != gf_isom_add_sample(fo, newTk, descIndex, tmp)) goto A;
                gf_isom_sample_del(&tmp);
              }
              if (0 == (tmp = gf_isom_get_sample(fi, data, sdte->sampleNumber, 0))) goto R;
              last_smp = sdte->sampleNumber;
            }
            if (D->P[l].lost && !(l == 0 && msubtype == 'avc1')) {
              if (mode & FILL && sdte->dataLength > 8) {
                memset(tmp->data + sdte->byteOffset + 8, 0, sdte->dataLength - 8);
              } else if (mode & TRUNC) {
              }                
            }
          }
        }
      }
    }
    if (esd) {
      rate += smp->dataLength;
      esd->decoderConfig->avgBitrate += smp->dataLength;
      if (esd->decoderConfig->bufferSizeDB < smp->dataLength) esd->decoderConfig->bufferSizeDB = smp->dataLength;
      if (smp->DTS - pos > ts) {
        if (esd->decoderConfig->maxBitrate < rate) esd->decoderConfig->maxBitrate = rate;
        rate = 0;
        pos = 0;
      }
    }
    gf_isom_sample_del(&smp);
  }

  if (msubtype == GF_ISOM_SUBTYPE_MPEG4_CRYP) {
    esd = gf_isom_get_esd(fi, data, 1);
  } else if (msubtype == GF_ISOM_SUBTYPE_AVC_H264) {
    gf_isom_set_pl_indication(fo, GF_ISOM_PL_VISUAL, 15);
    goto FINISH;
  } else if (!esd) {
    gf_isom_remove_root_od(fo);
    goto FINISH;
  }

  if (0 == (dur = (u32) gf_isom_get_media_duration(fo, newTk))) dur = ts;
  esd->decoderConfig->maxBitrate *= 8;
  esd->decoderConfig->avgBitrate *= 8 * ts / dur;
  gf_isom_change_mpeg4_description(fo, newTk, 1, esd);

  iod = (GF_InitialObjectDescriptor *) gf_isom_get_root_od(fo);
  switch (esd->decoderConfig->streamType) {
    case GF_STREAM_VISUAL:
      if (iod && (iod->tag == GF_ODF_IOD_TAG)) {
        gf_isom_set_pl_indication(fo, GF_ISOM_PL_VISUAL, iod->visual_profileAndLevel);
      } else if (esd->decoderConfig->objectTypeIndication == 32) {
        GF_M4VDecSpecInfo dsi;
        gf_m4v_get_config(esd->decoderConfig->decoderSpecificInfo->data, esd->decoderConfig->decoderSpecificInfo->dataLength, &dsi);
        gf_isom_set_pl_indication(fo, GF_ISOM_PL_VISUAL, dsi.VideoPL);
      } else {
        gf_isom_set_pl_indication(fo, GF_ISOM_PL_VISUAL, 0xFE);
      }
    default: break;
  }

FINISH:

    if (iod) gf_odf_desc_del((GF_Descriptor *)iod);
  if (esd) gf_odf_desc_del((GF_Descriptor *)esd);

  if (GF_OK != gf_isom_close(fo)) goto C;

  return 1;

E: fprintf(stderr, "Error parsing MPEG-4 VOP header\n"); goto X;
O: fprintf(stderr, "Couldn't open file %s: %s\n", fn, gf_error_to_string(gf_isom_last_error(0))); goto X;
R: fprintf(stderr, "Couldn't retrieve sample %d: %s\n", i + 1, gf_error_to_string(gf_isom_last_error(0))); goto X;
A: fprintf(stderr, "Couldn't add sample %d: %s\n", i + 1, gf_error_to_string(gf_isom_last_error(0))); goto X;
C: fprintf(stderr, "Couldn't close file %s: %s\n", fn, gf_error_to_string(gf_isom_last_error(0)));

X: if (fo) gf_isom_delete(fo);
   return 0;
}

static int write_damaged(GF_ISOFile *f, u32 hint, u32 data, char *fn, char *ext, data_t *D, MODE mode)
{
  if (!write_mp4(f, hint, data, strcat(fn, ".mp4"), D, mode)) {
    fprintf(stderr, "Couldn't export mp4-track %d: %s\n", hint, gf_error_to_string(gf_isom_last_error(0)));
  }
  if (!write_raw(f, hint, data, (strcpy(strrchr(fn, '.') + 1, ext), fn), D, mode)) {
    fprintf(stderr, "Couldn't export raw track %d: %s\n", data, gf_error_to_string(gf_isom_last_error(0)));
    return 0;
  }
  return 1;
}

int WriteMP4(char *s, char *d, data_t *D, MODE mode)
{
  GF_ISOFile *fi;
  u32 i, tracks, samples = 0, ref_track = 0, hint_track = 0, type, sub;
  char *ext = "";

  if (0 == (fi = gf_isom_open(s, GF_ISOM_OPEN_READ, 0))) {
    fprintf(stderr, "Couldn't open file %s: %s\n", s, gf_error_to_string(gf_isom_last_error(0)));
    return 0;
  }

  tracks = gf_isom_get_track_count(fi);
  for (i = 0; i < tracks; i++) {
    if (gf_isom_get_media_type(fi, i + 1) == 'hint') {
      if (GF_OK == gf_isom_get_reference(fi, i + 1, GF_ISOM_REF_HINT, 1, &ref_track)) {
        if (ref_track) {
          type = gf_isom_get_media_type(fi, ref_track);
          sub = gf_isom_get_mpeg4_subtype(fi, ref_track, 1);
          if (!sub) sub = gf_isom_get_media_subtype(fi, ref_track, 1);
          samples = gf_isom_get_sample_count(fi, ref_track);
          switch (sub) {
            case 'avc1': ext = "264"; break;
            case 'mp4v':
            case 'MPEG': ext = "m4v"; break;
            case 's263': ext = "263"; break;
            case 'mp4a': ext = "aac"; break;
            default    : ext = "dat";
          }
          if (mode & AUDIO) {
            if (type == 'soun') {
              hint_track = i + 1;
              break;
            }
          } else {
            if (type == 'vide') {
              hint_track = i + 1;
              break;
            }
          }
        }
      }
    }
  }

  if (!ref_track) {
    fprintf(stderr, "Couldn't find relevant %s track in %s.\n", mode & AUDIO ? "audio" : "video", s);
    return 0;
  }

  if (!(mode & AUDIO) && samples != D->nF) {
    fprintf(stderr, "Wrong number of samples in track %d.\n", ref_track);
    return 0;
  }
  if (!write_damaged(fi, hint_track, ref_track, d, ext, D, mode)) {
    fprintf(stderr, "Couldn't extract damaged track (%d).\n", ref_track);
    return 0;
  }

  return 1;
}
