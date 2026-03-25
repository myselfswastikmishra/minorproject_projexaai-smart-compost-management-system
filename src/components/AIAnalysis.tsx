import { CheckCircle2, AlertTriangle, Clock, Leaf } from "lucide-react";
import ReactMarkdown from "react-markdown";

interface Props {
  analysis: string | null;
}

export default function AIAnalysis({ analysis }: Props) {
  if (!analysis) {
    return (
      <div className="sensor-card flex flex-col items-center justify-center py-12 text-center">
        <Leaf className="h-12 w-12 text-muted-foreground/30 mb-3" />
        <p className="text-muted-foreground">Enter your sensor readings above to get AI-powered compost analysis</p>
      </div>
    );
  }

  const isSuitable = analysis.toLowerCase().includes("suitable") && !analysis.toLowerCase().includes("not suitable") && !analysis.toLowerCase().includes("unsuitable");

  return (
    <div className={`sensor-card border-l-4 ${isSuitable ? "border-l-primary" : "border-l-accent"}`}>
      <div className="flex items-center gap-2 mb-4">
        {isSuitable ? (
          <CheckCircle2 className="h-5 w-5 text-primary" />
        ) : (
          <AlertTriangle className="h-5 w-5 text-accent" />
        )}
        <h2 className="text-xl font-display font-bold text-foreground">
          {isSuitable ? "Conditions Look Great!" : "Adjustments Needed"}
        </h2>
      </div>
      <div className="prose prose-sm max-w-none text-foreground/90 [&_h3]:font-display [&_h3]:text-foreground [&_li]:text-foreground/80 [&_strong]:text-foreground">
        <ReactMarkdown>{analysis}</ReactMarkdown>
      </div>
    </div>
  );
}
