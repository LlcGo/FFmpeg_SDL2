#include <SDL.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

int main(int argc, char* argv[]) {
	
	AVFormatContext* avf_context = NULL;
	int ret = avformat_open_input(&avf_context, "C:\\Users\\asus\\Desktop\\test.mp4", NULL, NULL);
	if (ret != 0)
	{
		SDL_Log("avformat_open_input error");
		return -1;
	}

	if (avformat_find_stream_info(avf_context, NULL) < 0)
	{
		SDL_Log("avformat_find_stream_info error");
		return -1;
	}

	int videoindex = 1;
	for (int i = 0; i < avf_context->nb_streams; i++)
	{
		if (avf_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
		}
	}

	const AVCodec* decoder_ret = NULL;
	av_find_best_stream(avf_context, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder_ret, 0);
	if (decoder_ret == NULL)
	{
		SDL_Log("av_find_best_stream error");
		return -1;
	}
	printf(decoder_ret->name);



	return 0;
}