import { Clock, Flame, Thermometer, Droplets, Wind } from "lucide-react";

interface Reading {
  pm25: number;
  mq135: number;
  temperature: number;
  humidity: number;
  timestamp: Date;
}

interface Props {
  readings: Reading[];
}

export default function SensorHistory({ readings }: Props) {
  if (readings.length === 0) return null;

  return (
    <div className="sensor-card">
      <h2 className="text-xl font-display font-bold text-foreground flex items-center gap-2 mb-4">
        <Clock className="h-5 w-5 text-primary" />
        Reading History
      </h2>
      <div className="space-y-3 max-h-64 overflow-y-auto">
        {readings.map((r, i) => (
          <div key={i} className="flex items-center justify-between bg-muted/50 rounded-lg px-4 py-2.5 text-sm">
            <span className="text-muted-foreground">{r.timestamp.toLocaleTimeString()}</span>
            <div className="flex gap-4 flex-wrap">
              <span className="flex items-center gap-1"><Wind className="h-3.5 w-3.5 text-compost-warm" />{r.pm25}</span>
              <span className="flex items-center gap-1"><Flame className="h-3.5 w-3.5 text-compost-warm" />{r.mq135} ppm</span>
              <span className="flex items-center gap-1"><Thermometer className="h-3.5 w-3.5 text-destructive" />{r.temperature}°C</span>
              <span className="flex items-center gap-1"><Droplets className="h-3.5 w-3.5 text-compost-green-light" />{r.humidity}%</span>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
