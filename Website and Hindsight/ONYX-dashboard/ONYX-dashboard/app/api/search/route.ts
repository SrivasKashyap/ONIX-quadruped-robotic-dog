import { NextResponse } from 'next/server'
import { supabase } from '../../../lib/supabase'

export async function GET(request: Request) {
  try {
    const { searchParams } = new URL(request.url)
    const source = searchParams.get('source')
    const destination = searchParams.get('destination')
    const trip = searchParams.get('trip')

    let query = supabase
      .from('defects')
      .select('*, trips(trip_number), defect_clusters(detection_count)')
      .order('detected_at', { ascending: false })

    if (trip && trip !== 'all') {
      query = query.eq('trip_id', trip)
    }

    const { data: defects, error } = await query

    if (error) {
      return NextResponse.json({ error: error.message }, { status: 500 })
    }

    let warning = null
    if (defects && defects.length > 10) {
      warning = 'High density of track anomalies detected in selected section.'
    }

    return NextResponse.json({ defects: defects || [], warning })
  } catch (err: any) {
    return NextResponse.json({ error: err.message || 'Internal Server Error' }, { status: 500 })
  }
}