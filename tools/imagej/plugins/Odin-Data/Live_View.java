import java.awt.*;
import ij.*;
import ij.gui.*;
import ij.process.*;
import ij.plugin.PlugIn;

import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;
import static org.zeromq.ZMQ.SUB;
import static org.zeromq.ZMQ.context;

/** This a prototype ImageJ plugin. */
public class Live_View implements PlugIn{

	public void run(String arg) {
		long start = System.currentTimeMillis();
		int w = 400, h = 400;
		ImageProcessor ip = new ShortProcessor(w, h);
		short[] pixels = (short[])ip.getPixels();
		int i = 0;
		for (int y = 0; y < h; y++) {
			short red = (short)((y * 255) / (h - 1));
			for (int x = 0; x < w; x++) {
				
				pixels[i++] = red;
			}
		}
		new ImagePlus("Live View", ip).show();
		IJ.showStatus("Time Taken: "+(System.currentTimeMillis()-start));
	 }

}