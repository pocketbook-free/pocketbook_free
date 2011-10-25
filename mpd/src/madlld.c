#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <linux/soundcard.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mad.h"
#include "bstdfile.h"
#include "madlld.h"
#include "inkview.h"
#include "inkinternal.h"

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

#if 0
#define DEBUG_INFO(x)	fprintf(stderr, "File:%s,Func:%s, Line=%d, %s\n", __FILE__, __FUNCTION__, __LINE__, x);
#else
#define DEBUG_INFO(x)
#endif

#define AUDIODEVICE "/dev/sound/dsp"

/* Should we use getopt() for command-line arguments parsing? */
#if (defined(unix) || defined (__unix__) || defined(__unix) || \
	 defined(HAVE_GETOPT)) \
	&& !defined(WITHOUT_GETOPT)
#include <unistd.h>
#define HAVE_GETOPT
#else
#include <ctype.h>
#undef HAVE_GETOPT
#endif

/* Which audio destination to use? */
static int use_alsa = 0;

/* Date related to playback with ALSA */

static snd_pcm_t *handle;
static char *buffer;
static snd_pcm_hw_params_t *params;
static snd_pcm_uframes_t frames;	// unsigned long
static int open_alsa(unsigned int frequency);
static int close_alsa();
static int find_alsa_dev();

static int alsa_is_open = 0;

#define NUM_OF_FRAMES (2048)

/****************************************************************************
 * Global variables.														*
 ****************************************************************************/

static iv_mpctl *mpc;
static int plcount=0;
static short *randoms=NULL;
static FILE *dsp_file;
static int dspfd;

/* This table represents the subband-domain filter characteristics. It
 * is initialized by the ParseArgs() function and is used as
 * coefficients against each subband samples when DoFilter is non-nul.
 */
mad_fixed_t	Filter[32];

/* DoFilter is non-nul when the Filter table defines a filter bank to
 * be applied to the decoded audio subbands.
 */
int			DoFilter=0;
int			http_streaming;

static double AMP[75] =
{

	0.001000,0.001122,0.001259,0.001413,0.001585,
	0.001778,0.001995,0.002239,0.002512,0.002818,
	0.003162,0.003548,0.003981,0.004467,0.005012,
	0.005623,0.006310,0.007079,0.007943,0.008913,
	0.010000,0.011220,0.012589,0.014125,0.015849,
	0.017783,0.019953,0.022387,0.025119,0.028184,
	0.031623,0.035481,0.039811,0.044668,0.050119,
	0.056234,0.063096,0.070795,0.079433,0.089125,
	0.100000,0.112202,0.125893,0.141254,0.158489,
	0.177828,0.199526,0.223872,0.251189,0.281838,
	0.316228,0.354813,0.398107,0.446684,0.501187,
	0.562341,0.630957,0.707946,0.794328,0.891251,
	1.000000,1.122018,1.258925,1.412538,1.584893,
	1.778279,1.995262,2.238721,2.511886,2.818383,
	3.162278,3.548134,3.981072,4.466836,5.011872,

};


// I2C control
#define I2CDEVICE "/dev/i2c-0"
#define I2C_RETRIES 0x0701
#define I2C_TIMEOUT 0x0702
#define SLAVE_ADDR 0x1a
#define I2C_RDWR    0x0707

#define SSM2603_HP_VOLUME 		0x02
#define SSM2603_HP_VOL_MASK 	0x7F

struct i2c_msg
{
  __u16 addr; 			/* slave address */
  __u16 flags;
  __u16 len;	
  __u8 *buf; 				/*message data pointer*/
};

struct i2c_rdwr_ioctl_data
{
  struct i2c_msg  *msgs;				 /*i2c_msg[] pointer*/
  int nmsgs;            				 /*i2c_msg Nums*/
};
//

void write_data(int fd, unsigned int reg, unsigned int value)
{
       int ret;	
	struct i2c_rdwr_ioctl_data ssm_msg;
	unsigned char buf[2]={0};       

       ssm_msg.nmsgs = 1;
       ssm_msg.msgs = (struct i2c_msg*) malloc(ssm_msg.nmsgs * sizeof(struct i2c_msg));

	if(!ssm_msg.msgs)
	{
		printf("Memory alloc error\n");
		return;
	}
	
	buf[0] = (reg<<1) | ((value>>8) & 0x0001);
	buf[1] = value & 0x00ff;

	(ssm_msg.msgs[0]).flags=0;
	(ssm_msg.msgs[0]).addr=(__u16)SLAVE_ADDR;
	(ssm_msg.msgs[0]).buf=buf;
	(ssm_msg.msgs[0]).len=2;

	ret=ioctl(fd, I2C_RDWR, &ssm_msg);

	if(ret<0)
	{
	  printf("\nret=%d",ret);
	  printf(" \n    Write error! lenth isnot right!\n");
	}
	
	free(ssm_msg.msgs);
}

void set_volumemax()
{
    system("/bin/setmaxvolume");

}

static void
set_frequency(long frequency)
{
	if (use_alsa == 1)
		return;

	if (ioctl(dspfd, SNDCTL_DSP_SPEED, &frequency) < 0) {
		printf("ERROR: %s device Unable to set frequency to %d\n",
		       AUDIODEVICE, (int) frequency);
		mpc->state = MP_STOPPED;
		fclose(dsp_file);
		exit(0);
	}
}

static void
set_bits(long bits)
{

	if (use_alsa == 1)
		return;

	if (ioctl(dspfd, SNDCTL_DSP_SAMPLESIZE, &bits) < 0) {
		printf("ERROR: %s device Unable to set bits to %d\n",
		       AUDIODEVICE, (int) bits);
		mpc->state = MP_STOPPED;
		fclose(dsp_file);
		exit(0);
	}
}

static void
set_channels(long n_channel)
{

	if (use_alsa == 1)
		return;

	if (ioctl(dspfd, SNDCTL_DSP_CHANNELS, &n_channel) < 0) {
		printf("ERROR: %s device Unable to set channels to %d\n",
		       AUDIODEVICE, (int) n_channel);
		mpc->state = MP_STOPPED;
		fclose(dsp_file);
		exit(0);
	}
}

static void open_dsp() {

	set_volumemax();
	/* Open the audio playback device */
	dsp_file = fopen(AUDIODEVICE, "wr");
	if (dsp_file == NULL) {
		printf("ERROR: Can't open output device '%s'\n", AUDIODEVICE);
		mpc->state = MP_STOPPED;
		exit(0);
	}	
	dspfd = fileno(dsp_file);

	set_channels(2);
	set_frequency(44100);
	set_bits(AFMT_S16_LE);
/*
	freg=(24<<16)|(13);
	if (ioctl(dspfd, SNDCTL_DSP_SETFRAGMENT, &freg) < 0) 
	{
		printf("ERROR: %s device Unable to set freg to %d\n", AUDIODEVICE, freg);
		mpc->state = MP_STOPPED;
		fclose(dsp_file);
		exit(0);
	}
*/

}


static void close_dsp() {

	fclose(dsp_file);

}

/****************************************************************************
 * Return an error string associated with a mad error code.					*
 ****************************************************************************/
/* Mad version 0.14.2b introduced the mad_stream_errorstr() function.
 * For previous library versions a replacement is provided below.
 */
#if (MAD_VERSION_MAJOR>=1) || \
    ((MAD_VERSION_MAJOR==0) && \
     (((MAD_VERSION_MINOR==14) && \
       (MAD_VERSION_PATCH>=2)) || \
      (MAD_VERSION_MINOR>14)))
#define MadErrorString(x) mad_stream_errorstr(x)
#else
static const char *MadErrorString(const struct mad_stream *Stream)
{
	switch(Stream->error)
	{
		/* Generic unrecoverable errors. */
		case MAD_ERROR_BUFLEN:
			return("input buffer too small (or EOF)");
		case MAD_ERROR_BUFPTR:
			return("invalid (null) buffer pointer");
		case MAD_ERROR_NOMEM:
			return("not enough memory");

		/* Frame header related unrecoverable errors. */
		case MAD_ERROR_LOSTSYNC:
			return("lost synchronization");
		case MAD_ERROR_BADLAYER:
			return("reserved header layer value");
		case MAD_ERROR_BADBITRATE:
			return("forbidden bitrate value");
		case MAD_ERROR_BADSAMPLERATE:
			return("reserved sample frequency value");
		case MAD_ERROR_BADEMPHASIS:
			return("reserved emphasis value");

		/* Recoverable errors */
		case MAD_ERROR_BADCRC:
			return("CRC check failed");
		case MAD_ERROR_BADBITALLOC:
			return("forbidden bit allocation value");
		case MAD_ERROR_BADSCALEFACTOR:
			return("bad scalefactor index");
		case MAD_ERROR_BADFRAMELEN:
			return("bad frame length");
		case MAD_ERROR_BADBIGVALUES:
			return("bad big_values count");
		case MAD_ERROR_BADBLOCKTYPE:
			return("reserved block_type");
		case MAD_ERROR_BADSCFSI:
			return("bad scalefactor selection info");
		case MAD_ERROR_BADDATAPTR:
			return("bad main_data_begin pointer");
		case MAD_ERROR_BADPART3LEN:
			return("bad audio data length");
		case MAD_ERROR_BADHUFFTABLE:
			return("bad Huffman table select");
		case MAD_ERROR_BADHUFFDATA:
			return("Huffman data overrun");
		case MAD_ERROR_BADSTEREO:
			return("incompatible block_type for JS");

		/* Unknown error. This switch may be out of sync with libmad's
		 * defined error codes.
		 */
		default:
			return("Unknown error code");
	}
}
#endif

/****************************************************************************
 * Converts a sample from libmad's fixed point number format to a signed	*
 * short (16 bits).															*
 ****************************************************************************/
static signed short MadFixedToSshort(mad_fixed_t Fixed)
{
	/* A fixed point number is formed of the following bit pattern:
	 *
	 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * MSB                          LSB
	 * S ==> Sign (0 is positive, 1 is negative)
	 * W ==> Whole part bits
	 * F ==> Fractional part bits
	 *
	 * This pattern contains MAD_F_FRACBITS fractional bits, one
	 * should alway use this macro when working on the bits of a fixed
	 * point number. It is not guaranteed to be constant over the
	 * different platforms supported by libmad.
	 *
	 * The signed short value is formed, after clipping, by the least
	 * significant whole part bit, followed by the 15 most significant
	 * fractional part bits. Warning: this is a quick and dirty way to
	 * compute the 16-bit number, madplay includes much better
	 * algorithms.
	 */

	/* Clipping */
	if(Fixed>=MAD_F_ONE)
		return(SHRT_MAX);
	if(Fixed<=-MAD_F_ONE)
		return(-SHRT_MAX);

	/* Conversion. */
	Fixed=Fixed>>(MAD_F_FRACBITS-15);
	return((signed short)Fixed);
}

/****************************************************************************
 * Print human readable informations about an audio MPEG frame.				*
 ****************************************************************************/
static int PrintFrameInfo(FILE *fp, struct mad_header *Header)
{
	const char	*Layer,
				*Mode,
				*Emphasis;

	/* Convert the layer number to it's printed representation. */
	switch(Header->layer)
	{
		case MAD_LAYER_I:
			Layer="I";
			break;
		case MAD_LAYER_II:
			Layer="II";
			break;
		case MAD_LAYER_III:
			Layer="III";
			break;
		default:
			Layer="(unexpected layer value)";
			break;
	}

	/* Convert the audio mode to it's printed representation. */
	switch(Header->mode)
	{
		case MAD_MODE_SINGLE_CHANNEL:
			Mode="single channel";
			break;
		case MAD_MODE_DUAL_CHANNEL:
			Mode="dual channel";
			break;
		case MAD_MODE_JOINT_STEREO:
			Mode="joint (MS/intensity) stereo";
			break;
		case MAD_MODE_STEREO:
			Mode="normal LR stereo";
			break;
		default:
			Mode="(unexpected mode value)";
			break;
	}

	/* Convert the emphasis to it's printed representation. Note that
	 * the MAD_EMPHASIS_RESERVED enumeration value appeared in libmad
	 * version 0.15.0b.
	 */
	switch(Header->emphasis)
	{
		case MAD_EMPHASIS_NONE:
			Emphasis="no";
			break;
		case MAD_EMPHASIS_50_15_US:
			Emphasis="50/15 us";
			break;
		case MAD_EMPHASIS_CCITT_J_17:
			Emphasis="CCITT J.17";
			break;
#if (MAD_VERSION_MAJOR>=1) || \
	((MAD_VERSION_MAJOR==0) && (MAD_VERSION_MINOR>=15))
		case MAD_EMPHASIS_RESERVED:
			Emphasis="reserved(!)";
			break;
#endif
		default:
			Emphasis="(unexpected emphasis value)";
			break;
	}

#if 0
	fprintf(fp,"%s: %lu kb/s audio MPEG layer %s stream %s CRC, "
			"%s with %s emphasis at %d Hz sample rate\n",
			ProgName,Header->bitrate,Layer,
			Header->flags&MAD_FLAG_PROTECTION?"with":"without",
			Mode,Emphasis,Header->samplerate);
#endif			
	return(ferror(fp));
}

/****************************************************************************
 * Applies a frequency-domain filter to audio data in the subband-domain.	*
 ****************************************************************************/
static void ApplyFilter(struct mad_frame *Frame)
{
	int	Channel,
		Sample,
		Samples,
		SubBand;

	/* There is two application loops, each optimized for the number
	 * of audio channels to process. The first alternative is for
	 * two-channel frames, the second is for mono-audio.
	 */
	Samples=MAD_NSBSAMPLES(&Frame->header);
	if(Frame->header.mode!=MAD_MODE_SINGLE_CHANNEL)
		for(Channel=0;Channel<2;Channel++)
			for(Sample=0;Sample<Samples;Sample++)
				for(SubBand=0;SubBand<32;SubBand++)
					Frame->sbsample[Channel][Sample][SubBand]=
						mad_f_mul(Frame->sbsample[Channel][Sample][SubBand],
								  Filter[SubBand]);
	else
		for(Sample=0;Sample<Samples;Sample++)
			for(SubBand=0;SubBand<32;SubBand++)
				Frame->sbsample[0][Sample][SubBand]=
					mad_f_mul(Frame->sbsample[0][Sample][SubBand],
							  Filter[SubBand]);
}

/***********************************************************************************************
*								Function: VolumeSet ¥\¯à
*===============================================================================================
* 					Description: Set Volume
*					param:			
*						vol:volume
*					return: no	
************************************************************************************************/
static void VolumeSet()
{
	int i, v;

	DoFilter=1;
	for(i=0;i<32;i++) {
		v = mpc->volume + mpc->equalizer[i];
		if (v < -15) v = -15;
		if (v > 51) v = 51;
		Filter[i]= (mpc->volume == 0) ? 0 : mad_f_tofixed(AMP[v+15]);
	}
}

/****************************************************************************
 * Main decoding loop. This is where mad is used.							*
 ****************************************************************************/
#define INPUT_BUFFER_SIZE	(5*NUM_OF_FRAMES*4)
#define OUTPUT_BUFFER_SIZE	(NUM_OF_FRAMES*4)	/* Must be an integer multiple of 4. */
//#define INPUT_BUFFER_SIZE     (5*512)
//#define OUTPUT_BUFFER_SIZE    512 /* Must be an integer multiple of 4. */

/***********************************************************************************************
*								Function: MpegAudioDecoder ¥\¯à
*===============================================================================================
* 					Description: Main decoding loop. This is where mad is used.
*					param:			
*						InputFp: a file descriptor for music file
						OutputFp:a file descriptor for hardware
*					return: 1	
************************************************************************************************/
static int MpegAudioDecoder(FILE *InputFp, FILE *OutputFp)
{
	struct mad_stream	Stream;
	struct mad_frame	Frame;
	struct mad_synth	Synth;
	mad_timer_t			Timer;
	static unsigned char		InputBuffer[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD],
						OutputBuffer[OUTPUT_BUFFER_SIZE],
						*OutputPtr=OutputBuffer,
						*GuardPtr=NULL;
	const unsigned char	*OutputBufferEnd=OutputBuffer+OUTPUT_BUFFER_SIZE;
	int					Status=0,i;
	unsigned long		FrameCount=0;
	bstdfile_t			*BstdFile;
	int starting;
	
	printf("Enter MpegAudioDecoder\n");
	DEBUG_INFO("Enter MpegAudioDecoder");
	/* First the structures used by libmad must be initialized. */
	DEBUG_INFO("mad_stream_init");
	mad_stream_init(&Stream);
	DEBUG_INFO("mad_frame_init");	
	mad_frame_init(&Frame);
	DEBUG_INFO("mad_synth_init");	
	mad_synth_init(&Synth);
	DEBUG_INFO("mad_timer_reset");	
	mad_timer_reset(&Timer);

	/* Decoding options can here be set in the options field of the
	 * Stream structure.
	 */

	/* {1} When decoding from a file we need to know when the end of
	 * the file is reached at the same time as the last bytes are read
	 * (see also the comment marked {3} bellow). Neither the standard
	 * C fread() function nor the POSIX read() system call provides
	 * this feature. We thus need to perform our reads through an
	 * interface having this feature, this is implemented here by the
	 * bstdfile.c module.
	 */
	DEBUG_INFO("NewBstdFile");		 
	BstdFile=NewBstdFile(InputFp);
	if(BstdFile==NULL)
	{
		fprintf(stderr,"error creating bstdfile\n");
		return(1);
	}

	VolumeSet();
	mpc->newposition = -1;
	starting = 1;

	/* This is the decoding loop. */
	do
	{
		if (mpc->filter_changed) {
			VolumeSet();
			mpc->filter_changed = 0;
		}

		if (mpc->newposition != -1) {
			BstdSeek(BstdFile, mpc->newposition, SEEK_SET);
			mpc->newposition = -1;
			Stream.error = MAD_ERROR_BUFLEN;
			Stream.next_frame = NULL;
			starting = 1;
		}

		mpc->position = BstdTell(BstdFile);

		//find = 0;
		/* The input bucket must be filled if it becomes empty or if
		 * it's the first execution of the loop.
		 */
		if(Stream.buffer==NULL || Stream.error==MAD_ERROR_BUFLEN)
		{
			size_t			ReadSize,
							Remaining;
			unsigned char	*ReadStart;

			/* {2} libmad may not consume all bytes of the input
			 * buffer. If the last frame in the buffer is not wholly
			 * contained by it, then that frame's start is pointed by
			 * the next_frame member of the Stream structure. This
			 * common situation occurs when mad_frame_decode() fails,
			 * sets the stream error code to MAD_ERROR_BUFLEN, and
			 * sets the next_frame pointer to a non NULL value. (See
			 * also the comment marked {4} bellow.)
			 *
			 * When this occurs, the remaining unused bytes must be
			 * put back at the beginning of the buffer and taken in
			 * account before refilling the buffer. This means that
			 * the input buffer must be large enough to hold a whole
			 * frame at the highest observable bit-rate (currently 448
			 * kb/s). XXX=XXX Is 2016 bytes the size of the largest
			 * frame? (448000*(1152/32000))/8
			 */
			DEBUG_INFO("");
			if(Stream.next_frame!=NULL)
			{
				Remaining=Stream.bufend-Stream.next_frame;
				memmove(InputBuffer,Stream.next_frame,Remaining);
				ReadStart=InputBuffer+Remaining;
				ReadSize=INPUT_BUFFER_SIZE-Remaining;
			}
			else
				ReadSize=INPUT_BUFFER_SIZE,
					ReadStart=InputBuffer,
					Remaining=0;

			/* Fill-in the buffer. If an error occurs print a message
			 * and leave the decoding loop. If the end of stream is
			 * reached we also leave the loop but the return status is
			 * left untouched.
			 */
			DEBUG_INFO("");
			ReadSize=BstdRead(ReadStart,1,ReadSize,BstdFile);
			if(ReadSize<=0)
			{
				break;
			}

			/* {3} When decoding the last frame of a file, it must be
			 * followed by MAD_BUFFER_GUARD zero bytes if one wants to
			 * decode that last frame. When the end of file is
			 * detected we append that quantity of bytes at the end of
			 * the available data. Note that the buffer can't overflow
			 * as the guard size was allocated but not used the the
			 * buffer management code. (See also the comment marked
			 * {1}.)
			 *
			 * In a message to the mad-dev mailing list on May 29th,
			 * 2001, Rob Leslie explains the guard zone as follows:
			 *
			 *    "The reason for MAD_BUFFER_GUARD has to do with the
			 *    way decoding is performed. In Layer III, Huffman
			 *    decoding may inadvertently read a few bytes beyond
			 *    the end of the buffer in the case of certain invalid
			 *    input. This is not detected until after the fact. To
			 *    prevent this from causing problems, and also to
			 *    ensure the next frame's main_data_begin pointer is
			 *    always accessible, MAD requires MAD_BUFFER_GUARD
			 *    (currently 8) bytes to be present in the buffer past
			 *    the end of the current frame in order to decode the
			 *    frame."
			 */
			DEBUG_INFO(""); 
			if(BstdFileEofP(BstdFile))
			{
				GuardPtr=ReadStart+ReadSize;
				memset(GuardPtr,0,MAD_BUFFER_GUARD);
				ReadSize+=MAD_BUFFER_GUARD;
			}

			/* Pipe the new buffer content to libmad's stream decoder
             * facility.
			 */
			DEBUG_INFO(""); 
			mad_stream_buffer(&Stream,InputBuffer,ReadSize+Remaining);
			Stream.error=0;
			if (starting && !use_alsa) {
				starting = 0;
				usleep(250000);
			}
		}

		/* Decode the next MPEG frame. The streams is read from the
		 * buffer, its constituents are break down and stored the the
		 * Frame structure, ready for examination/alteration or PCM
		 * synthesis. Decoding options are carried in the Frame
		 * structure from the Stream structure.
		 *
		 * Error handling: mad_frame_decode() returns a non zero value
		 * when an error occurs. The error condition can be checked in
		 * the error member of the Stream structure. A mad error is
		 * recoverable or fatal, the error status is checked with the
		 * MAD_RECOVERABLE macro.
		 *
		 * {4} When a fatal error is encountered all decoding
		 * activities shall be stopped, except when a MAD_ERROR_BUFLEN
		 * is signaled. This condition means that the
		 * mad_frame_decode() function needs more input to complete
		 * its work. One shall refill the buffer and repeat the
		 * mad_frame_decode() call. Some bytes may be left unused at
		 * the end of the buffer if those bytes forms an incomplete
		 * frame. Before refilling, the remaining bytes must be moved
		 * to the beginning of the buffer and used for input for the
		 * next mad_frame_decode() invocation. (See the comments
		 * marked {2} earlier for more details.)
		 *
		 * Recoverable errors are caused by malformed bit-streams, in
		 * this case one can call again mad_frame_decode() in order to
		 * skip the faulty part and re-sync to the next frame.
		 */
		DEBUG_INFO("mad_frame_decode");		 
		if(mad_frame_decode(&Frame,&Stream))
		{
			DEBUG_INFO("");
			if(MAD_RECOVERABLE(Stream.error))
			{
				/* Do not print a message if the error is a loss of
				 * synchronization and this loss is due to the end of
				 * stream guard bytes. (See the comments marked {3}
				 * supra for more informations about guard bytes.)
				 */
				if(Stream.error!=MAD_ERROR_LOSTSYNC ||
				   Stream.this_frame!=GuardPtr)
				{
					printf("fflush\n");
					MadErrorString(&Stream);
					fflush(stderr);
					printf("fflush ok\n");
				}
				continue;
			}
			else
				if(Stream.error==MAD_ERROR_BUFLEN)
					continue;
				else
				{
					MadErrorString(&Stream);
					Status=1;
					break;
				}
		}

		/* The characteristics of the stream's first frame is printed
		 * on stderr. The first frame is representative of the entire
		 * stream.
		 */
		if(FrameCount==0)
		{
			DEBUG_INFO("PrintFrmaeInfo");
			if(PrintFrameInfo(stderr,&Frame.header))
			{
				Status=1;
				break;
			}
		}
		/* Accounting. The computed frame duration is in the frame
		 * header structure. It is expressed as a fixed point number
		 * whole data type is mad_timer_t. It is different from the
		 * samples fixed point format and unlike it, it can't directly
		 * be added or subtracted. The timer module provides several
		 * functions to operate on such numbers. Be careful there, as
		 * some functions of libmad's timer module receive some of
		 * their mad_timer_t arguments by value!
		 */
		FrameCount++;
		mad_timer_add(&Timer,Frame.header.duration);

		/* Between the frame decoding and samples synthesis we can
		 * perform some operations on the audio data. We do this only
		 * if some processing was required. Detailed explanations are
		 * given in the ApplyFilter() function.
		 */
		if(DoFilter)
			ApplyFilter(&Frame);

		/* Once decoded the frame is synthesized to PCM samples. No errors
		 * are reported by mad_synth_frame();
		 */
		DEBUG_INFO("mad_synth_frame"); 
		mad_synth_frame(&Synth,&Frame);

		if (!alsa_is_open && use_alsa) open_alsa(Synth.pcm.samplerate);
		set_frequency(Synth.pcm.samplerate);

		/* Synthesized samples must be converted from libmad's fixed
		 * point number to the consumer format. Here we use unsigned
		 * 16 bit big endian integers on two channels. Integer samples
		 * are temporarily stored in a buffer that is flushed when
		 * full.
		 */
		//printf("Synth len %d \n",Synth.pcm.length);
		for(i=0;i<Synth.pcm.length;i++)
		{
			signed short	Sample;
				
			if (mpc->state != MP_PLAYING) break;

			/* Left channel */
			//DEBUG_INFO("");
			Sample=MadFixedToSshort(Synth.pcm.samples[0][i]);
			//*(OutputPtr++)=Sample>>8;
			*(OutputPtr++)=Sample&0xff;
			*(OutputPtr++)=Sample>>8;

			/* Right channel. If the decoded stream is monophonic then
			 * the right output channel is the same as the left one.
			 */
			if(MAD_NCHANNELS(&Frame.header)==2)
				Sample=MadFixedToSshort(Synth.pcm.samples[1][i]);
			*(OutputPtr++)=Sample&0xff;
			*(OutputPtr++)=Sample>>8;


			/* Flush the output buffer if it is full. */
			if (OutputPtr == OutputBufferEnd) {
				
				if (use_alsa == 1) {
					int rc;

					rc = snd_pcm_writei(handle, OutputBuffer, NUM_OF_FRAMES);
					DEBUG_INFO("");
					if (rc == -EPIPE) {
						/*  EPIPE means underrun */
						fprintf(stderr, "underrun occurred\n");
						snd_pcm_prepare(handle);
						Status = 2;
						break;
					} else if (rc < 0) {
						fprintf(stderr,
							"error from writei: %s\n",
							snd_strerror(rc));
						Status = 2;
						break;
					} else if (rc != (int) NUM_OF_FRAMES) {
						fprintf(stderr,
							"short write, write %d frames\n",
							rc);
						Status = 2;
						break;
					}
					OutputPtr = OutputBuffer;
				}
				else {
					if (fwrite (OutputBuffer, 1, OUTPUT_BUFFER_SIZE, OutputFp) != OUTPUT_BUFFER_SIZE) {
						fprintf(stderr,"PCM write error (%s)\n", strerror(errno));
						Status = 2;
						break;
					}
					OutputPtr = OutputBuffer;
				}
			}
		}
		DEBUG_INFO("Synth.pcm.length");

		if (mpc->state == MP_PAUSED) {
			ioctl(dspfd, SNDCTL_DSP_RESET, 0);
			use_alsa ? close_alsa() : close_dsp();
			//close_alsa();
			//close_dsp();
			while (mpc->state == MP_PAUSED) {
				usleep(50000);
			}
			find_alsa_dev();
			use_alsa ? open_alsa(Synth.pcm.samplerate) : open_dsp();
			//open_alsa();
			//open_dsp();
		}

	} while (mpc->state == MP_PLAYING || mpc->state == MP_PAUSED);

	/* The input file was completely read; the memory allocated by our
	 * reading module must be reclaimed.
	 */
	DEBUG_INFO("Quit Synth.pcm.length, BstdFileDestroy"); 
	BstdFileDestroy(BstdFile);

	/* Mad is no longer used, the structures that were initialized must
     * now be cleared.
	 */
	DEBUG_INFO("mad_synth_finish"); 	 
	mad_synth_finish(&Synth);
	mad_frame_finish(&Frame);
	mad_stream_finish(&Stream);

	/* If the output buffer is not empty and no error occurred during
     * the last write, then flush it.
	 */
	if (OutputPtr != OutputBuffer && Status != 2
	    && mpc->state == MP_PLAYING) {
		size_t BufferSize = OutputPtr - OutputBuffer;

		if (use_alsa == 1) {
			int rc;

			rc = snd_pcm_writei(handle, OutputBuffer, BufferSize / 4);
			if (rc == -EPIPE) {
				/*  EPIPE means underrun */
				fprintf(stderr, "underrun occurred\n");
				snd_pcm_prepare(handle);
				Status = 2;
			} else if (rc < 0) {
				fprintf(stderr, "error from writei: %s\n",
					snd_strerror(rc));
				Status = 2;
			} else if (rc != (int) BufferSize) {
				fprintf(stderr, "short write, write %d frames\n", rc);
				Status = 2;
			}
		} 
		else {
			if (fwrite(OutputBuffer, 1, BufferSize, OutputFp) != BufferSize) {
				fprintf(stderr, "PCM write error (%s)\n",
					strerror(errno));
				Status = 2;
			}
		}
	}

	/* Accounting report if no error occurred. */
	if(!Status)
	{
		char	Buffer[80];

		/* The duration timer is converted to a human readable string
		 * with the versatile, but still constrained mad_timer_string()
		 * function, in a fashion not unlike strftime(). The main
		 * difference is that the timer is broken into several
		 * values according some of it's arguments. The units and
		 * fracunits arguments specify the intended conversion to be
		 * executed.
		 *
		 * The conversion unit (MAD_UNIT_MINUTES in our example) also
		 * specify the order and kind of conversion specifications
		 * that can be used in the format string.
		 *
		 * It is best to examine libmad's timer.c source-code for details
		 * of the available units, fraction of units, their meanings,
		 * the format arguments, etc.
		 */
		mad_timer_string(Timer,Buffer,"%lu:%02lu.%03u",
						 MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);
		#if 0						 
		fprintf(stderr,"%s: %lu frames decoded (%s).\n",
				ProgName,FrameCount,Buffer);
		#endif
	}

	/* That's the end of the world (in the H. G. Wells way). */
	return(Status);
}

static int random_next(int n) {

	int i;

	for (i=0; i<plcount; i++) {
		if (randoms[i] == n)
			return randoms[(i+1) % plcount];
	}
	return 0;

}

static int random_prev(int n) {

	int i;

	for (i=0; i<plcount; i++) {
		if (randoms[i] == n)
			return randoms[(i+plcount-1) % plcount];
	}
	return 0;

}


static char *nextline(FILE *f) {

	static char buf[1024];
	char *p, *pp;

	while (fgets(buf, sizeof(buf)-1, f) != NULL) {
		p = buf;
		while (*p == ' ' || *p == '\t') p++;
		pp = p + strlen(p);
		while (pp > p && (*(pp-1) & 0xe0) == 0) pp--;
		*pp = 0;
		if (*p == 0) continue;
		return p;
	}
	return NULL;

}

static int get_playlist_size() {

	FILE *f;
	int n=0;

	f = fopen(PLAYLISTFILE, "r");
	if (f == NULL) return 0;
	while (nextline(f) != NULL) n++;
	fclose(f);
	return n;

}

static char *get_playlist_filename(int trackno) {

	FILE *f;
	char *s;
	int n=0;

	f = fopen(PLAYLISTFILE, "r");
	if (f == NULL) return NULL;
	while ((s = nextline(f)) != NULL) {
		if (n == trackno) {
			fclose(f);
			return s;
		}
		n++;
	}
	fclose(f);
	return NULL;

}

static int
open_alsa(unsigned int frequency)
{
	int rc;
	int size;
	unsigned int val;
	int dir = 0;

    rc = snd_pcm_open(&handle, "plug:a2dpd", SND_PCM_STREAM_PLAYBACK, 0);
//	rc = snd_pcm_open(&handle, buf, SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle, params);

	rc = snd_pcm_hw_params_set_rate_resample(handle,params,1);
	if (rc < 0) {
		fprintf(stderr, "unable to set resample :: snd_pcm_hw_params_set_rate_resample :: %s\n", snd_strerror(rc));
	}

	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(handle, params, 2);

	if (frequency > 96000) {
		fprintf(stderr, "Can`t play! Bad sample rate = %ui\n", frequency);
		exit(1);
	}
	snd_pcm_hw_params_set_rate_near(handle, params, &frequency, &dir);

	frames = NUM_OF_FRAMES;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n",
			snd_strerror(rc));
		exit(1);
	}
	alsa_is_open = 1;
	return 1;
}

static int
close_alsa()
{
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	alsa_is_open = 0;
	return 1;
}

static int
find_alsa_dev()
{
	int ret = 0;

	ret = system(NETAGENT " a2dp s");
	switch(ret) {
		case -1:
			fprintf(stderr, "Vilka provalilas'\n");
			break;
		default:
			switch (WEXITSTATUS(ret)) {
				case A2DP_DISCONNECTED:
					fprintf(stderr, "NO A2DP devices were found\n");
					use_alsa = 0;
					break;
				case A2DP_CONNECTED_TO_SNK:
					fprintf(stderr, "Found A2DP device\n");
					use_alsa = 1;
					break;
				default:
					fprintf(stderr, "Some error happaned\n");
					break;
			}
			ret = 1;
			break;
	}
	return ret;
}

/***********************************************************************************************
*								Function: play mp3 ¥\¯à
*===============================================================================================
* 					Description: play music
*					param:			
*						Mp3Filename : music name path
*					return: 1	
************************************************************************************************/

int main(int argc, char **argv)
{
	FILE *f;
	char *filename;
	int mpc_shm;
	int i, j, v;
	char streambuf[256];

	mpc_shm = shmget(0xa12302, sizeof(iv_mpctl), IPC_CREAT | 0666);
	if (mpc_shm == -1) {
		fprintf(stderr, "cannot get shared memory segment\n");
		return 1;
	}
	mpc = (iv_mpctl *) shmat(mpc_shm, NULL, 0);
	if (mpc == (void*)-1) {
		fprintf(stderr, "cannot attach shared memory segment\n");
		return 1;
	}
	fprintf(stderr, "Atached mp shm: id %x addr %x\n", mpc_shm, (int)mpc);
	mpc->state = MP_STOPPED;
	mpc->volume = 10;

	find_alsa_dev();

	while (1) {

		switch (mpc->state) {
			case MP_STOPPED:
				fprintf(stderr, "\nMP_STOPPED\n");
				break;
			case MP_REQUEST_FOR_PLAY:
				fprintf(stderr, "\nMP_REQUEST_FOR_PLAY\n");
				break;
			case MP_PLAYING:
				fprintf(stderr, "\nMP_PLAYING\n");
				break;
			case MP_PAUSED:
				fprintf(stderr, "\nMP_PAUSED\n");
				break;
			case MP_PREVTRACK:
				fprintf(stderr, "\nMP_PREVTRACK\n");
				break;
			case MP_NEXTTRACK:
				fprintf(stderr, "\nMP_NEXTTRACK\n");
				break;
			default:
				fprintf(stderr, "\nMP_UNKNOWN\n");
				break;
		}

		while (mpc->state == MP_STOPPED)
			usleep(100000);

		find_alsa_dev();

		if (mpc->state == MP_REQUEST_FOR_PLAY) {
			fprintf(stderr, "request for play: track %i\n", mpc->track);
			plcount = get_playlist_size();
			if (plcount < 1) {
				mpc->state = MP_STOPPED;
				continue;
			}
			srand(time(NULL));
			if (randoms) free(randoms);
			randoms = (short *) malloc(plcount * sizeof(short));
			for (i=0; i<plcount; i++) randoms[i] = i;
			for (i=0; i<plcount; i++) {
				j = rand() % plcount;
				v = randoms[i];
				randoms[i] = randoms[j];
				randoms[j] = v;
			}
			mpc->state = MP_PLAYING;
		}

		filename = get_playlist_filename(mpc->track);
		if (filename == NULL) {
			fprintf(stderr, "null filename\n");
			mpc->state = MP_STOPPED;
			continue;
		}

		f = fopen(filename, "rb");
		if (f != NULL) {
			fseek(f, 0, SEEK_END);
			mpc->tracksize = ftell(f);
			mpc->position = 0;
			fseek(f, 0, SEEK_SET);
			for (i=0; i<500; i++) fread(streambuf, 1, 256, f);
			fseek(f, 0, SEEK_SET);
			fprintf(stderr, "playing %s\n", filename);
			if (!use_alsa) open_dsp();
			//open_dsp();
			//open_alsa();
			MpegAudioDecoder(f, dsp_file);
			fclose(f);
			use_alsa ? close_alsa() : close_dsp();
			//close_dsp();
			//close_alsa();
		} else {
			fprintf(stderr, "cannot open %s\n", filename);
			sleep(1);
		}

		if (mpc->state == MP_STOPPED || mpc->state == MP_REQUEST_FOR_PLAY) continue;

		plcount = get_playlist_size();
		if (plcount < 1) {
			mpc->state = MP_STOPPED;
			continue;
		}

		if (mpc->state == MP_PREVTRACK) {
			if (mpc->mode != MP_RANDOM) {
				mpc->track--;
				if (mpc->track < 0) mpc->track = plcount-1;
				if (mpc->track < 0) mpc->track = 0;
			} else {
				mpc->track = random_prev(mpc->track);
			}
			fprintf(stderr, "previous track: %i\n", mpc->track);
			mpc->state = MP_PLAYING;
			continue;
		}

		if (mpc->state == MP_NEXTTRACK) {
			if (mpc->mode != MP_RANDOM) {
				mpc->track++;
				if (mpc->track >= plcount) mpc->track = 0;
			} else {
				mpc->track = random_next(mpc->track);
			}
			fprintf(stderr, "next track: %i\n", mpc->track);
			mpc->state = MP_PLAYING;
			continue;
		}

		switch (mpc->mode) {
			default:
				mpc->state = MP_STOPPED;
				break;
			case MP_CONTINUOUS:
				mpc->track++;
				if (mpc->track >= plcount) mpc->track = 0;
				break;
			case MP_RANDOM:
				mpc->track = random_next(mpc->track);
				break;
		}

	}

}
