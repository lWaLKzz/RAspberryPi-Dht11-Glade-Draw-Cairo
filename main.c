#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cairo.h>
#include <cairo.h> 
#include <gtk/gtk.h>

#define pinNumber 29
#define DHTLIB_OK				0
#define DHTLIB_ERROR_CHECKSUM	-1
#define DHTLIB_ERROR_TIMEOUT	-2

typedef unsigned char uint8;
typedef unsigned int  uint16;
typedef unsigned long uint32;
int humidity;
int temperature;
GtkBuilder      *builder; 
GtkWidget       *window;
GtkLabel		*label;
GtkWidget       *area;

gboolean time_handler(GtkLabel *label);
void setup(void);
uint8 readSensorData(int pin);
long map(long x, long in_min, long in_max, long out_min, long out_max);
//Thread
PI_THREAD (dht11)
{
	while(1){
		if(readSensorData(pinNumber)){
			//rh = (databuf>>24)&0xff;
			//rh2 = (databuf>>16)&0xff;
			//tmp = (databuf>>8)&0xff;
			//tmp2 = databuf&0xff;
			//printf("RH : %d.%d\n",rh,rh2);
			//printf("TMP:%d.%d\n",(databuf>>8)&0xff,databuf&0xff);
			printf("%d %d\n",humidity,temperature);
		}
		delay(1000);
	}
}
gboolean on_areaDraw_draw(GtkDrawingArea *widget,cairo_t *cr){
	GtkWidget *win = gtk_widget_get_toplevel(window);
	int width, height;
	gtk_window_get_size(GTK_WINDOW(win), &width, &height);
	int crh = map(humidity,0,100,0,width/2);
	int ctmp = map(temperature,0,100,0,width/2);
	
	cairo_move_to (cr, 20, 13);
	cairo_line_to (cr, crh , 13);
	

	cairo_move_to (cr, 20, 33);
	cairo_line_to (cr, ctmp, 33);
	
	
	cairo_set_line_width (cr, 10.0);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_stroke (cr);
	
	return FALSE;
}

int main(int argc, char *argv[])
{	
    
	setup();
    gtk_init(&argc, &argv);
    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "gtkWin.glade", NULL);
    window = GTK_WIDGET(gtk_builder_get_object(builder, "win"));
    label = GTK_LABEL(gtk_builder_get_object(builder, "setLabel"));
    area = GTK_WIDGET(gtk_builder_get_object(builder, "areaDraw")); 
    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(builder);
    gtk_widget_show(window);
    g_timeout_add(20, (GSourceFunc)time_handler,label);           
    gtk_main();
 
    
    return 0;
}

gboolean time_handler(GtkLabel *label) {
	char update[50];
	sprintf(update,"Humidity : %d\nTemperature : %d",humidity,temperature);
	gtk_label_set_text(label,update);
	gtk_widget_queue_draw(area);
    return TRUE;
}

void setup(void){
	wiringPiSetup();
	piThreadCreate(dht11);
	
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
uint8 readSensorData(int pin)
{
	uint8_t bits[5];
	uint8_t cnt = 7;
	uint8_t idx = 0;

	// EMPTY BUFFER
	for (int i=0; i< 5; i++) bits[i] = 0;

	// REQUEST SAMPLE
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
	delay(18);
	digitalWrite(pin, HIGH);
	delayMicroseconds(40);
	pinMode(pin, INPUT);

	// ACKNOWLEDGE or TIMEOUT
	unsigned int loopCnt = 10000;
	while(digitalRead(pin) == LOW)
		if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;

	loopCnt = 10000;
	while(digitalRead(pin) == HIGH)
		if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;

	// READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
	for (int i=0; i<40; i++)
	{
		loopCnt = 10000;
		while(digitalRead(pin) == LOW)
			if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;

		unsigned long t = micros();

		loopCnt = 10000;
		while(digitalRead(pin) == HIGH)
			if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;

		if ((micros() - t) > 40) bits[idx] |= (1 << cnt);
		if (cnt == 0)   // next byte?
		{
			cnt = 7;    // restart at MSB
			idx++;      // next byte!
		}
		else cnt--;
	}

	// WRITE TO RIGHT VARS
        // as bits[1] and bits[3] are allways zero they are omitted in formulas.
	humidity    = bits[0]; 
	temperature = bits[2]; 

	uint8_t sum = bits[0] + bits[2];  

	if (bits[4] != sum) return DHTLIB_ERROR_CHECKSUM;
	return DHTLIB_OK;
}
