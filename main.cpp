#include <SDL.h>
#include <windows.h>
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

	SetConsoleOutputCP(CP_UTF8);   // 让控制台按 UTF-8 解释输出
    SetConsoleCP(CP_UTF8);         // 输入也设为 UTF-8（可选）
	
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

	//int videoindex = 1;
	//for (int i = 0; i < avf_context->nb_streams; i++)
	//{
	//	if (avf_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	//	{
	//		videoindex = i;
	//	}
	//}

	const AVCodec* decoder_ret = NULL;
	int videoindex = av_find_best_stream(avf_context, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder_ret, 0);
	if (decoder_ret == NULL)
	{
		SDL_Log("av_find_best_stream error");
		return -1;
	}
	//printf(decoder_ret->name);

	// 寻找解码器
	const AVCodec* avcodec_dec = avcodec_find_decoder(avf_context->streams[videoindex]->codecpar->codec_id);
	if (avcodec_dec == NULL)
	{
		SDL_Log("avcodec_find_decoder error");
		return -1;
	}

	// 创建AVCodecContext拷贝流
	AVCodecContext* av_codec_context = avcodec_alloc_context3(avcodec_dec);
	if (avcodec_parameters_to_context(av_codec_context, avf_context->streams[videoindex]->codecpar) < 0)
	{
		SDL_Log("avcodec_parameters_to_context error");
		return -1;
	}

	// 打开解码器
	if (avcodec_open2(av_codec_context, avcodec_dec, NULL) != 0)
	{
		SDL_Log("avcodec_open2 error");
		return -1;
	}

	// AV_FORMAT_INPUT 封装格式 相关
	printf("av input format = %s\n",avf_context->iformat->name);
	printf("视频时长 = %d\n", avf_context->duration);
	printf("输入视频总的码率（视频、音频、字幕等） = %d\n", avf_context->bit_rate);
	printf("封装格式长名称 = %s \n", avf_context->iformat->long_name);
	printf("封装格式扩展名 = %s \n", avf_context->iformat->extensions);

	// 编解码相关
	// AVStreams 个数
	printf("av streams = %d\n", avf_context->nb_streams);
	// 输入视频 AVStream 数组
	printf("输入视频 AVStream 数组 = %d\n", avf_context->streams);
	for (int i = 0; i < avf_context->nb_streams;i++)
	{
		// 流内的信息
		printf("avf_context->streams id = %d \n", avf_context->streams[i]->id);
		printf("avf_context->streams[i]->codecpar->codec_type  = %d \n", avf_context->streams[i]->codecpar->codec_type);
		printf("avf_context->streams[i]->time_base  = %d \n", avf_context->streams[i]->time_base);
		printf("流帧率  = %d \n", avf_context->streams[i]->r_frame_rate);
	}

	// 编解码器信息
	printf("编解码器 => \n", av_codec_context->codec);
	printf("图像width = %d,height = %d\n", av_codec_context->width, av_codec_context->height);
	printf("像素格式 = %d\n", av_codec_context->pix_fmt);
	printf("采样率 = %d\n", av_codec_context->sample_rate);
	printf("声道数 = %d\n", av_codec_context->ch_layout.nb_channels);
	printf("采样格式 = %d\n", av_codec_context->sample_fmt);
	printf("编码器名字 = %s\n", avcodec_dec->name);
	printf("编码器长名字 = %s\n", avcodec_dec->long_name);
	printf("编码器类型 = %d\n", avcodec_dec->type);
	printf("编码器ID = %d\n", avcodec_dec->id);

	// AVPacket
	printf("时间戳 => %d \n", avf_context->streams[videoindex]->attached_pic.pts);
	printf("解码器 => %d \n", avf_context->streams[videoindex]->attached_pic.dts);
	printf("压缩数据 => %d \n", avf_context->streams[videoindex]->attached_pic.data);
	printf("数据大小 => %d \n", avf_context->streams[videoindex]->attached_pic.size);
	printf("所属的AVStream => %d \n", avf_context->streams[videoindex]->attached_pic.stream_index);

	// 1. 分配 packet 和 frame
	AVPacket* pkt = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();   // ← AVFrame 在这里

	// 2. 循环读 packet
	while (av_read_frame(avf_context, pkt) >= 0) {
		if (pkt->stream_index == videoindex) {
			// 3. 送 packet 给解码器
			avcodec_send_packet(av_codec_context, pkt);

			// 4. 收解码出来的帧
			while (avcodec_receive_frame(av_codec_context, frame) == 0) {
				// ★ 这里 frame 里才有真正的数据
				// frame->data[0], frame->data[1], frame->data[2]
				// frame->linesize[0..2]
				// frame->width, frame->height, frame->format
				printf("解码出一帧: %dx%d, format=%d\n",
					frame->width, frame->height, frame->format);
			}
		}
		av_packet_unref(pkt);
	}

	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}