from pandas_datareader import data
from datetime import datetime, timedelta
import paho.mqtt.client as mqtt
import stock

STOCK_SYMBL = 'TSLA'
mqtt_server = "192.168.0.000" #< change this to your MQTT server
mqtt_port = 1883

def on_connect(client, userdata, flags, rc):
        print("Connected with result code "+str(rc))

#retrive data from IEX
start = datetime.now() - timedelta(days=5*365)
stockd = data.DataReader(STOCK_SYMBL, 'iex', start)
stockd.to_csv('stockdata.csv')

#Stock Prediction
stockq = stock.initialize()
yesclose = stockd.iloc[-1].tolist()[3]
resulthigh = stockq.predhigh('stockdata.csv')
resultlow = stockq.predlow('stockdata.csv')
resultopen = stockq.predopen('stockdata.csv')
resultclose = stockq.pred('stockdata.csv')
print "Predicted Open : " + str("%.2f" % resultopen)
print "Predicted Close: " + str("%.2f" % resultclose)
print "Yesterday Close: " + str("%.2f" % yesclose)
print "Predicted High : " + str("%.2f" % resulthigh)
print "Predicted Low  : " + str("%.2f" % resultlow)

#Connect MQTT
client = mqtt.Client()
client.on_connect = on_connect
client.connect(mqtt_server, mqtt_port, 60)
client.loop_start()

#Publish Data
client.publish("home/stock/yclose"   , yesclose   , 0, True);
client.publish("home/stock/predhigh" , resulthigh , 0, True);
client.publish("home/stock/predlow"  , resultlow  , 0, True);
client.publish("home/stock/predopen" , resultopen , 0, True);
client.publish("home/stock/predclose", resultclose, 0, True);

#Stop MQTT
client.loop_stop()
client.disconnect()
