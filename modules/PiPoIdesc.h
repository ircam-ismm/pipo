/**
 * @file PiPoIdesc.h
 * @author Diemo.Schwarz@ircam.fr
 *
 * @brief PiPo wrapper for maxpipo project using idesc ircamdescriptor API
 *
 * @copyright
 * Copyright (C) 2012 by IMTR IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 */

#ifndef _PIPO_IDESC_
#define _PIPO_IDESC_

// debug print
#define IDESC_DEBUG (DEBUG*1)

#include "PiPo.h"
#include <vector>
#include <map>
#include <stdexcept>


#ifdef WIN32
#define snprintf sprintf_s
#endif

//#define post printf
#define IDESC_REAL_TYPE float	/* pipo with mubu works with float output data */
#include "idesc.h"

using namespace std;

class idescx;

class PiPoIdesc : public PiPo
{
public:
  // define parameter fields (= attributes)
#define IDESC_PARAM(TYPE, NAME, DEFAULT, BLURB)	\
  PiPoScalarAttr<TYPE> NAME;
#define  IDESC_BANDS_READ(NAME, ATTR, MAXSIZE, BLURB)	\
  PiPoVarSizeAttr<float> ATTR;
#include "ircamdescriptor~params.h"

  PiPoScalarAttr<enum PiPo::Enumerate> window;
  PiPoScalarAttr<enum PiPo::Enumerate> windowunit;
  PiPoVarSizeAttr<enum PiPo::Enumerate> descriptors; // list of descriptor name symbols asked for by user

  PiPoIdesc(PiPo::Parent *parent, PiPo *receiver = NULL);
  ~PiPoIdesc(void);

  int streamAttributes(bool hasTimeTags, double rate, double offset,
                       unsigned int width, unsigned int size, const char **labels,
                       bool hasVarSize, double domain, unsigned int maxFrames);
  int finalize (double inputEnd);
  int reset(void);
  int frames(double time, double weight, PiPoValue *values, unsigned int size, unsigned int num);

private:
  static void datacallback (int descrid, int varnum, int numval, IDESC_REAL_TYPE *values, void* obj);
  static void endcallback (double frame_time_sec, void* obj);

  idescx		       *idesc_ = NULL;
  std::vector<PiPoValue>        outbuf_;
  bool				initialised_ = false;
  int				status_ = -1;
  std::map<int, int>		doffset_; // maps idesc-internal descr. id to start index in output columns
  std::map<int, int>		dwidth_;  // maps idesc-internal descr. id to number of output columns
  int				ndescr_requested_ = -1; // length of descriptors attr.
  int				numcols_ = 0; // total number of output columns
  const char                  **colnames_;

  void clearcolnames();
};


class idescx : public idesc::idesc
{
public:
  idescx (double sr, double winsize, double hopsize, PiPoIdesc *_pipo)
  : idesc(sr, winsize, hopsize),
    pipo(_pipo)
  {
    this->sr = sr;
    this->winsize = winsize;
    this->hopsize = hopsize;
  };

  // dummy setters to keep IDESC_PARAM macro happy
  void set_WindowSize (int ws) {};
  void set_HopSize (int hs) {};

  double get_WindowSize() {return winsize;};
  double get_HopSize() {return hopsize;};
  double get_sr() {return sr;};
  
  // set chroma limits as one band: fixed num = 1!
  void set_chromarange_band_limits (int num, float *bands)
  {
    if (num == 1)
      set_ChromaRange(bands[0], bands[1]);
  };

  /** always returns 1 */
  int  get_chromarange_band_num () { return 1; };

  /** get chroma limits as one band */
  void get_chromarange_band_limits (/*out*/ float *bands)
  { // get limits from first and last band
    bands[0] = pipo->ChromaRange.getDbl(0);
    bands[1] = pipo->ChromaRange.getDbl(1);
  };

private:
  PiPoIdesc *pipo;	// pointer to containing object to query chroma range
  double sr;
  double winsize;
  double hopsize;
}; // end class idescx



PiPoIdesc::PiPoIdesc(PiPo::Parent *parent, PiPo *receiver)
: PiPo(parent, receiver),

// declare and initialise parameter fields (= attributes)
#define IDESC_PARAM(TYPE, NAME, DEFAULT, BLURB)	\
  NAME(this, #NAME, BLURB, true, DEFAULT),	// "changesStream" is true for most of them
#define IDESC_BANDS_READ(NAME, ATTR, MAXSIZE, BLURB)	\
  ATTR(this, #ATTR, BLURB, true),
#include "ircamdescriptor~params.h"

  window(this, "window", "Analysis window type", false, 0),
  windowunit(this, "windowunit", "Analysis window and hop size unit", true, 0),
  descriptors(this, "descriptors", "Descriptors to calculate", true, 0)
{
  idesc_  = NULL;
  colnames_ = NULL;

  // set up and query idesc library
  idesc::idesc::init_library();	// no deinit_library needed within max session
  int num_descr_available = idesc::idesc::get_num_descriptors();

  // set up descriptor enum (relying on it starting at 0)
  for (int i = 0; i < num_descr_available; i++)
    descriptors.addEnumItem(idesc::idesc::get_descriptor_name(i));

  // set up window type and unit enums
  window.addEnumItem("blackman");
  window.addEnumItem("hamming");
  window.addEnumItem("hanning");
  window.addEnumItem("hanning2");
  windowunit.addEnumItem("source");
  windowunit.addEnumItem("resampled");
  windowunit.addEnumItem("msec");

  // get default band limit lists to populate attrs
# define IDESC_BANDS_READ(NAME, ATTR, MAXSIZE, BLURB)			\
  ATTR.setSize(idesc_ ? 2 * idesc_->get_ ## NAME ## _band_num(): 0);	\
  if (idesc_) idesc_->get_ ## NAME ## _band_limits(ATTR.getPtr());

# include "ircamdescriptor~params.h"

  initialised_ = true;
} // end ctor PiPoIdesc::PiPoIdesc ()

PiPoIdesc::~PiPoIdesc(void)
{
  if (idesc_)
  {
    delete idesc_;
    idesc_ = NULL;
  }
  if (colnames_ != NULL)
    clearcolnames();	//FIXME: free strings
}

void PiPoIdesc::clearcolnames ()
{
  for (int i = 0; i < numcols_; i++)
    free((void *) colnames_[i]);

  free((void *) colnames_);
  colnames_ = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// init module
//

int PiPoIdesc::streamAttributes (bool hasTimeTags, double rate, double offset,
                                 unsigned int width, unsigned int size,
                                 const char **labels, bool hasVarSize,
                                 double domain, unsigned int maxFrames)
{
  double factor;

  switch (windowunit.get())
  {
    default:
    case 0:	factor = rate;			break;  // relative to input sr
    case 1:	factor = ResampleTo.get();	break;  // relative to resampled sr
    case 2:	factor = 1000;			break;  // in milliseconds
  }

  double winlen = WindowSize.getDbl() / factor;	// window and hop size in sec
  double hoplen = HopSize.getDbl()    / factor;	// hop size in sec
  int    ndescr = descriptors.getSize(); // length of @descriptors attr list

#if IDESC_DEBUG >= 3
  printf("PiPoIdesc streamAttributes timetags %d  rate %.0f  offset %f  width %d  size %d  labels %s  "
       "varsize %d  domain %f  maxframes %d --> win %d = %f s  hop %d = %f s  numdescr %d\n",
       hasTimeTags, rate, offset, (int) width, (int) size, labels ? labels[0] : "n/a", (int) hasVarSize, (float) domain, (int) maxFrames,
       (int) WindowSize.getInt(), (float) winlen, (int) HopSize.getInt(), (float) hoplen, ndescr);
#endif

  if (initialised_  &&  ndescr > 0)
  {
    try {
      // init idesc
      if (idesc_ != NULL  &&  (idesc_->get_sr() != rate  ||  idesc_->get_WindowSize() != winlen  ||  idesc_->get_HopSize() != hoplen
                               || ndescr != ndescr_requested_)) // workaround for probable bug in idesc lib: changing number of descr. does not work (previous descr. output stays), see #240, #439
      {
	delete idesc_;	// reinit only first time or if params changed
	idesc_ = NULL;
      }
      
      if (idesc_ == NULL)
      {
#if IDESC_DEBUG >= 2
	printf("PiPoIdesc reinit numdescr %d\n", ndescr);
#endif
	idesc_ = new idescx(rate, winlen, hoplen, this);
      }

      ndescr_requested_ = ndescr;
      numcols_ = 0; // number of output columns (>= ndescr)

      // set up idesc params from pipo attrs
#     define IDESC_PARAM(TYPE, NAME, DEFAULT, BLURB) \
      idesc_->set_ ## NAME(NAME.get());
#     define IDESC_BANDS_WRITE(NAME, ATTR, MAXSIZE, BLURB)		\
      { int sz = ATTR.getSize();					\
	if (sz > 1) idesc_->set_ ## NAME ## _band_limits(sz / 2, ATTR.getPtr()); /* at least 2 */ \
	if (sz & 1) signalWarning(#ATTR " needs pairs of values, truncating"); \
      }
      /* As it is not possible with pipoattrs to get called directly
       when an attr changes, the following semantics is not
       possible: when num bands are set explicitly, the user-set
       bands are reinitialised to default values.
       The only way to clear user-defined bands is not to set them as attributes, or to set them with 0 length.
       #        define IDESC_BANDN_UPDATED(_name, _bands) \
       */

#     include "ircamdescriptor~params.h"

      if (colnames_ != NULL) clearcolnames();
      colnames_ = (const char **) malloc(ndescr * sizeof(char *));

      // set window type
      idesc_->set_window(window.getStr());

      // set up unique list of idesc descriptors
      int ndescr_unique = 0;
      int ndescr_dropped = 0;
      std::map<int, int> descr_seen;
      
      for (int i = 0; i < ndescr; i++)
      {
        const char *dname = descriptors.getStr(i - ndescr_dropped);
        int         did   = descriptors.getInt(i - ndescr_dropped);

        if (dname  &&  did >= 0)
        {
	  if (descr_seen.count(did) == 0)
	  { // first occurrence in list
	    descr_seen[did] = ndescr_unique;
	    colnames_[ndescr_unique++] = strdup(dname);
	    idesc_->set_descriptor(did, idesc::idesc::get_default_variation(did));
	  }
	  else
	  { // was already in list: ignore for idesc, remove from attr list (will be ndescr_dropped shorter)
	    descriptors.remove(i - ndescr_dropped);
	    ndescr_dropped++;
	    signalWarning(std::string("double occurence of ") +dname+ " in descriptor attribute was removed");
	  }
	}
        else
        {
          throw std::invalid_argument(std::string("unknown descriptor name at index ") + std::to_string(i)); //C++11
        }

#if IDESC_DEBUG >= 2
        printf("colnames descr %2d/%2d: %s (%d)\n", i, ndescr, colnames_[i], did);
        //post("colnames descr %2d/%2d: %s\n", i, ndescr, colnames_[i]);
#endif
      }
      ndescr = ndescr_unique;

      // build idesc graph
      idesc_->build_descriptors();

      // query output sizes
      for (int i = 0; i < ndescr; i++)
      {
        int did   = descriptors.getInt(i);
	int dcols = idesc_->get_dimensions(did);
	dwidth_[did]  = dcols;    // number of output columns of user-specified descr. i
	doffset_[did] = numcols_; // start index output columns of user-specified descr. i
	numcols_ += dcols;
#if IDESC_DEBUG >= 2
        printf("%2d: did %2d  doffset %2d  dwidth %2d  numcols %2d\n", i, did, dwidth_[did], doffset_[did], numcols_);
#endif
      }
      outbuf_.resize(numcols_);

      if (numcols_ > ndescr) // at least one descr has more than 1 output columns
      { // generate column names with index for non-singleton descriptors
        colnames_ = (const char **) realloc(colnames_, numcols_ * sizeof(char *));
        for (int i = ndescr - 1; i >= 0; i--)
        {
          int did = descriptors.getInt(i); // get internal descr id
          int dwidth  = dwidth_[did];
          int doffset = doffset_[did];
          //post("  descr %d width %d -> col %d (numcols %d): %s\n", i, dwidth, doffset, numcols_, colnames_[i]);

          if (dwidth == 1)
            colnames_[doffset] = colnames_[i];	// one output column: move strdup'ed string pointer
          else
          { // several output columns: generate labels as name+index
            char nbuf[128];
            int  digits = width > 9  ?  2  :  1; // one or two digits?

            for (int j = dwidth - 1; j >= 0; j--)
            {
              snprintf(nbuf, 128, "%s%0*d", colnames_[i], digits, j);
              colnames_[doffset + j] = strdup(nbuf);

              //post("    colname %d: %s\n", doffset + j, colnames_[doffset + j]);
            }
          }
        }
      }

      bool varsize = false; // HarmonicModel in descriptors?
      status_ = 0;

      return this->propagateStreamAttributes(true, 1 / hoplen, offset, numcols_, 1,
                                             colnames_, varsize, winlen * 1000., 1);
    } catch (std::exception& e) {
#if IDESC_DEBUG > 0
      post("pipo.ircamdescriptor error: IrcamDescriptor library: %s\n", e.what ());
#endif
      signalError(std::string("pipo.ircamdescriptor error: IrcamDescriptor library: ") + e.what());

      if (idesc_)
      {
        delete idesc_;
        idesc_ = NULL;
      }
      status_ = -1;
      return -1;
    }
  }
  else
    return -1; // TBD: signalWarning?
} // end streamAttributes ()


int PiPoIdesc::finalize (double inputEnd)
{
#if IDESC_DEBUG >= 2
  post("PiPoIdesc finalize %f\n", inputEnd);
#endif
  return this->propagateFinalize(inputEnd);
};


int PiPoIdesc::reset (void)
{
#if IDESC_DEBUG >= 2
  post("PiPoIdesc reset\n");
#endif

  if (idesc_)
  {
    /*try {
      // rebuild idesc graph to start over (necessary after finalize)
      // assume sizes have not changed
      idesc_->build_descriptors();
    } catch (std::exception& e) {
#if IDESC_DEBUG > 0
      post("pipo.ircamdescriptor reset error: IrcamDescriptor library: %s\n", e.what ());
#endif
      signalError(std::string("pipo.ircamdescriptor reset error: IrcamDescriptor library: ") + e.what());
      return -1;
    }*/

    status_ = 0;

    return this->propagateReset();
  }
  else
    return -1;
};


///////////////////////////////////////////////////////////////////////////////
//
// compute and output data
//

// called by idesc lib for every descriptor computed: write into pipo output frame
void PiPoIdesc::datacallback (int descrid, int varnum, int numval,
                              IDESC_REAL_TYPE *values, void* obj)
{
  PiPoIdesc *self = (PiPoIdesc *) obj;

  int offset = self->doffset_[descrid];
  int num    = self->dwidth_[descrid];

#if DEBUG
  if (offset + num > self->outbuf_.size())
  {
    printf("idesc datacallback overflow: descrid %d varnum %d numval %d doffset %d dwidth %d outsize %d\n", descrid, varnum, numval, offset, num, self->outbuf_.size());
    num = std::max<int>(0, self->outbuf_.size() - offset);
  }
#endif
  for (int i = 0; i < num; i++)
    self->outbuf_[offset + i] = values[i];
} // end datacallback ()

// called by idesc lib after all descriptor were computed and transmitted in datacallback: write propagate pipo output frame
void PiPoIdesc::endcallback (double frame_time_sec, void* obj)
{
  PiPoIdesc *self = (PiPoIdesc *) obj;

  // propagate gathered frame data
  self->status_ = self->propagateFrames(frame_time_sec * 1000., 1., self->outbuf_.data(), self->numcols_, 1);
}

int PiPoIdesc::frames (double time, double weight, PiPoValue *values, unsigned int size, unsigned int num)
{
#if IDESC_DEBUG >= 2
  post("PiPoIdesc::frames time %f  values %p  size %d  num %d\n",
       time, values, size, num);
#endif
  float *mono;

  if (idesc_)
  {
    if (size > 1)
    { // pick mono channel
      mono = (float *) alloca(num * sizeof(float));

      for (unsigned int i = 0, j = 0; i < num; i++, j += size)
        mono[i] = values[j];

      values = mono;
    }

    try {
      idesc_->compute(num, values, datacallback, NULL, endcallback, this);
    } catch (std::exception& e) {
#if IDESC_DEBUG >= 2
      post("pipo.ircamdescriptor frames error: IrcamDescriptor library: %s\n", e.what ());
      printf("pipo.ircamdescriptor frames error: IrcamDescriptor library: %s\n", e.what ());
#endif
      signalError(std::string("pipo.ircamdescriptor frames error: IrcamDescriptor library: ") + e.what());
      return -1;
    }
    
    return status_;
  }
  else
    return -1;
} // end frames ()

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset:2
 * End:
 */

#endif /* _PIPO_IDESC_ */
