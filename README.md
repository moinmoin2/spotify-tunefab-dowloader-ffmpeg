spotify-tunefab-dowloader-ffmpeg
================================

## Idea and Approach

I found a way to pass the 3 Minute limit of the TuneFab Spotify Converter by modifying ffmpeg source code


I have analyzed the way how the Tunefab Spotify Converter works and how its generating the mp3 and other audio formats. 
![Screenshot 2022-08-07 172905](https://user-images.githubusercontent.com/48869696/183298555-e3516433-04a9-4678-b897-21822081a1fa.png)

First a couple of other things happen then ffmpeg gets started.

![Screenshot 2022-08-07 173127](https://user-images.githubusercontent.com/48869696/183298661-2151aacd-3541-469c-89e2-42c5c9dce15b.png)

Then the raw audio file gets downloaded and then converted using ffmpeg to mp3

Here you can see the full command thats been executed 

* ` C:\Windows\system32\cmd.exe /d /s /c ""C:\Program Files\TuneFab Spotify Music Converter\ffmpeg.exe" -y -f f32le -channels 2 -ss 00:00:00 -t 180 -i "C:\Users\WDAGUtilityAccount\TuneFab\Spotify Music Converter\Converted\0.pcm" -i "C:\Users\WDAGUtilityAccount\TuneFab\Spotify Music Converter\Converted\0.jpg" -s 300x300 -map 0:0 -map 1:0 -disposition:v:0 attached_pic -c:v:0 mjpeg -f "mp3" -ar "44100" -ab "256k" -id3v2_version 3 -metadata title="Beautiful Girl" -metadata artist="Luciano" -metadata album="Beautiful Girl" -metadata track="1" -metadata album_artist="Luciano" "C:\Users\WDAGUtilityAccount\TuneFab\Spotify Music Converter\Converted\Beautiful Girl.mp3""

So the time limit of the free version is 3 minutes or 180 seconds and here they use the "-t 180" to shorten the song while converting to 3 minutes. So my idea was if I could remove the "-t 180" I could step over the 3 minute barrier. But I couldn't modify the executed parameters so I had to modify the source code of ffmpeg, where I ignore the argument. So I decided I would overrite the 3 numbers after the "-t" to 000 or 999 so ffmpeg does not shorten the song anymore. Unfortunatly I'm not really able to get the modification done. I'm not shure if I put the modification at the right spot. Maybye someone can help

My modification to the cmdutils.c
![Screenshot 2022-08-07 175319](https://user-images.githubusercontent.com/48869696/183299548-ceec03d0-dae1-433c-b9de-d83a19858dd8.png)

## Compile

You can recompile this repo by downloading it and type in bash in the folder 

* ` /configure --disable-shared --enable-static --disable-asm --enable-debug.

to configure make. Then you simply type "make" to compile it from source. Then you get a ffmpeg and a ffmpeg_g, the last one is bigger which is used for debbuging using gdb or gdb in visual studio code.


Thank you



