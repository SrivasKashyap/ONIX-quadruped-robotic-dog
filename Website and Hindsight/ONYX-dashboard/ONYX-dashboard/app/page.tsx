import data from './data.json';

export default function Dashboard() {
  return (
    <div className="min-h-screen bg-gray-900 text-gray-100 p-8 font-sans">
      <header className="mb-10 border-b border-gray-700 pb-6">
        <div className="flex justify-between items-end">
          <div>
            <h1 className="text-4xl font-extrabold text-blue-400 mb-2">RailGuard AI</h1>
            <p className="text-gray-400">Live track monitoring with persistent Hindsight memory</p>
          </div>
          <div className="text-right">
            <span className="inline-block bg-green-900 text-green-300 text-xs px-3 py-1 rounded-full font-bold uppercase tracking-wide">
              Prototype Online
            </span>
            <p className="text-xs text-gray-500 mt-2">Memory-informed assessment. Not a safety clearance.</p>
          </div>
        </div>
      </header>

      <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-8">
        {data.map((record) => (
          <div key={record.event.event_id} className="bg-gray-800 rounded-xl border border-gray-700 shadow-2xl overflow-hidden flex flex-col">
            
            <div className="relative h-64 bg-black border-b border-gray-700">
              <img
                src={`/${record.images.annotated}`}
                alt={record.event.defect_type}
                className="w-full h-full object-contain"
              />
              <div className="absolute top-3 right-3 bg-red-600/90 text-white text-xs px-3 py-1.5 rounded shadow border border-red-500 font-mono font-bold">
                CONFIDENCE: {(record.event.detection_score * 100).toFixed(0)}%
              </div>
            </div>

            <div className="p-5 flex-grow flex flex-col">
              <div className="flex justify-between items-start mb-4">
                <div>
                  <h2 className="text-xl font-bold text-gray-100">{record.event.event_id}</h2>
                  <p className="text-sm text-gray-400 mt-1 uppercase tracking-wider">{record.event.defect_type.replace(/_/g, ' ')}</p>
                </div>
                <span className="text-xs bg-yellow-900/50 text-yellow-400 px-3 py-1 rounded border border-yellow-700/50 font-medium">
                  {record.review_status.replace(/_/g, ' ')}
                </span>
              </div>

              <div className="grid grid-cols-2 gap-3 text-sm text-gray-400 mb-6 bg-gray-900/50 p-3 rounded border border-gray-700/50">
                <div><span className="text-gray-500">Trip:</span> {record.event.trip_id}</div>
                <div><span className="text-gray-500">Segment:</span> {record.event.segment_id}</div>
                <div><span className="text-gray-500">Time:</span> {new Date(record.event.timestamp).toLocaleTimeString()}</div>
                <div><span className="text-gray-500 text-xs italic">{record.location_note}</span></div>
              </div>

              <div className="mt-auto bg-blue-900/20 border border-blue-800/50 rounded-lg p-4">
                <h3 className="text-sm font-bold text-blue-400 flex items-center gap-2 mb-2 uppercase tracking-wide">
                  🧠 AI Memory Assessment
                </h3>
                <p className="text-sm text-blue-100/90 mb-4 leading-relaxed">
                  {record.explanation}
                </p>
                
                {record.memory_used && (
                  <div className="border-t border-blue-800/50 pt-3">
                    <p className="text-xs text-blue-500 font-bold mb-2 uppercase tracking-wider">Recalled Context:</p>
                    <ul className="text-xs text-gray-400 space-y-2">
                      {record.recalled_memories.slice(0, 2).map((mem: string, i: number) => (
                        <li key={i} className="flex items-start gap-2">
                          <span className="text-blue-500 mt-0.5">•</span>
                          <span className="line-clamp-2" title={mem}>{mem}</span>
                        </li>
                      ))}
                    </ul>
                  </div>
                )}
              </div>

              <div className="mt-4 bg-gray-700/40 p-3 rounded-lg text-sm text-center text-gray-300 font-medium border border-gray-600/50">
                Action: {record.suggested_action}
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}