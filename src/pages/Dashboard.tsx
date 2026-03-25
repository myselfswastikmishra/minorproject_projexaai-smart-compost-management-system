import { useState } from "react";
import Navbar from "@/components/Navbar";
import SensorForm from "@/components/SensorForm";
import AIAnalysis from "@/components/AIAnalysis";
import SensorHistory from "@/components/SensorHistory";
import ChatbotWidget from "@/components/ChatbotWidget";
import { supabase } from "@/integrations/supabase/client";
import { toast } from "sonner";
import { Flame, Leaf, TrendingUp, Thermometer, Droplets } from "lucide-react";

interface Reading {
  pm25: number;
  mq135: number;
  temperature: number;
  humidity: number;
  timestamp: Date;
}

export default function Dashboard() {
  const [analysis, setAnalysis] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [readings, setReadings] = useState<Reading[]>([]);

  const handleAnalyze = async (data: { pm25: number; mq135: number; temperature: number; humidity: number }) => {
    setLoading(true);
    try {
      const { data: result, error } = await supabase.functions.invoke("compost-chat", {
        body: {
          type: "analyze",
          sensorData: data,
        },
      });
      if (error) throw error;
      setAnalysis(result.response);
      setReadings(prev => [{ ...data, timestamp: new Date() }, ...prev].slice(0, 20));
    } catch (err: any) {
      toast.error("Failed to analyze: " + (err.message || "Unknown error"));
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-background">
      <Navbar />
      <main className="container mx-auto px-4 py-8 max-w-4xl">
        {/* Header */}
        <div className="mb-8 animate-fade-in">
          <h1 className="text-3xl font-display font-bold text-foreground mb-2">Compost Dashboard</h1>
          <p className="text-muted-foreground">Monitor your bioWaste composting process with real-time AI analysis</p>
        </div>

        {/* Quick stats */}
        {readings.length > 0 && (
          <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 mb-6 animate-fade-in">
            {[
              { label: "Last PM2.5", value: `${readings[0].pm25} µg/m³`, icon: TrendingUp, color: "text-compost-warm" },
              { label: "Last MQ-135", value: `${readings[0].mq135} ppm`, icon: Flame, color: "text-compost-warm" },
              { label: "Last Temp", value: `${readings[0].temperature}°C`, icon: Thermometer, color: "text-destructive" },
              { label: "Last Humidity", value: `${readings[0].humidity}%`, icon: Droplets, color: "text-compost-green-light" },
            ].map((stat) => (
              <div key={stat.label} className="sensor-card text-center py-4">
                <stat.icon className={`h-5 w-5 mx-auto mb-1 ${stat.color}`} />
                <p className="text-lg font-bold text-foreground">{stat.value}</p>
                <p className="text-xs text-muted-foreground">{stat.label}</p>
              </div>
            ))}
          </div>
        )}

        <div className="space-y-6">
          <SensorForm onSubmit={handleAnalyze} loading={loading} />
          <AIAnalysis analysis={analysis} />
          <SensorHistory readings={readings} />
        </div>
      </main>
      <ChatbotWidget />
    </div>
  );
}
