import { useState } from "react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Activity, Droplets, Flame, Loader2, Thermometer, Wind } from "lucide-react";

interface SensorData {
  pm25: number;
  mq135: number;
  temperature: number;
  humidity: number;
}

interface Props {
  onSubmit: (data: SensorData) => void;
  loading: boolean;
}

export default function SensorForm({ onSubmit, loading }: Props) {
  const [pm25, setPm25] = useState("");
  const [mq135, setMq135] = useState("");
  const [temperature, setTemperature] = useState("");
  const [humidity, setHumidity] = useState("");

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSubmit({
      pm25: parseFloat(pm25),
      mq135: parseFloat(mq135),
      temperature: parseFloat(temperature),
      humidity: parseFloat(humidity),
    });
  };

  return (
    <form onSubmit={handleSubmit} className="sensor-card space-y-5">
      <h2 className="text-xl font-display font-bold text-foreground flex items-center gap-2">
        <Activity className="h-5 w-5 text-primary" />
        Sensor Readings
      </h2>

      <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
        <div>
          <Label htmlFor="pm25" className="flex items-center gap-1.5 mb-1.5">
            <Wind className="h-3.5 w-3.5 text-compost-warm" />
            PM2.5 (µg/m³)
          </Label>
          <Input id="pm25" type="number" step="0.1" min="0" value={pm25} onChange={e => setPm25(e.target.value)} placeholder="e.g. 35" required />
        </div>
        <div>
          <Label htmlFor="mq135" className="flex items-center gap-1.5 mb-1.5">
            <Flame className="h-3.5 w-3.5 text-compost-warm" />
            MQ-135 (ppm)
          </Label>
          <Input id="mq135" type="number" step="0.1" min="0" value={mq135} onChange={e => setMq135(e.target.value)} placeholder="e.g. 200" required />
        </div>
        <div>
          <Label htmlFor="temp" className="flex items-center gap-1.5 mb-1.5">
            <Thermometer className="h-3.5 w-3.5 text-destructive" />
            Temperature (°C)
          </Label>
          <Input id="temp" type="number" step="0.1" min="-40" max="80" value={temperature} onChange={e => setTemperature(e.target.value)} placeholder="e.g. 55" required />
        </div>
        <div>
          <Label htmlFor="hum" className="flex items-center gap-1.5 mb-1.5">
            <Droplets className="h-3.5 w-3.5 text-compost-green-light" />
            Humidity (%)
          </Label>
          <Input id="hum" type="number" step="0.1" min="0" max="100" value={humidity} onChange={e => setHumidity(e.target.value)} placeholder="e.g. 60" required />
        </div>
      </div>

      <Button type="submit" className="w-full" disabled={loading}>
        {loading ? <Loader2 className="h-4 w-4 mr-2 animate-spin" /> : <Activity className="h-4 w-4 mr-2" />}
        Analyze Compost Conditions
      </Button>
    </form>
  );
}
